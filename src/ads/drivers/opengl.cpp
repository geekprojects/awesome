//
// Created by Ian Parker on 05/11/2020.
//

#include "opengl.h"
#include <awesome/displayserver.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

static unsigned int powerOfTwo(int input)
{
    unsigned int value = 1;

    while (value < input)
    {
        value <<= 1u;
    }
    return value;
}

OpenGLDisplayDriver::OpenGLDisplayDriver(const std::string& name,DisplayServer* displayServer) : DisplayDriver(name, displayServer)
{
}

OpenGLDisplayDriver::~OpenGLDisplayDriver() = default;

OpenGLDisplay::OpenGLDisplay(const std::string& name, OpenGLDisplayDriver* displayDriver) : Display(name, displayDriver)
{
}

OpenGLDisplay::~OpenGLDisplay() = default;

bool OpenGLDisplay::init()
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    // clear the drawing buffer.
    //glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // clear the identity matrix.
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    return true;
}

bool OpenGLDisplay::startDraw()
{
    setCurrentContext();

    glMatrixMode(GL_MODELVIEW);
    // clear the drawing buffer.
    glClear(GL_COLOR_BUFFER_BIT);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // clear the identity matrix.
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   glViewport(0, 0, m_rect.w * m_scale, m_rect.h * m_scale);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, (double)m_rect.w * m_scale, (double)m_rect.h * m_scale, 0.0, 0.0, 1.0);

    /* This allows alpha blending of 2D textures with the scene */

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //glBlendFunc(GL_ONE, GL_ONE);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    //glEnable(GL_ALPHA_TEST);
    //glAlphaFunc(GL_GREATER, 0);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    return true;
}

bool OpenGLDisplay::draw(Window* window, Geek::Rect drawRect)
{
    OpenGLWindowDisplayData* data = getData(window);
    Rect windowRect = window->getRect();

    if (data->frameTexture != nullptr && window->hasFrame())
    {
        updateTexture(data, data->frameTexture, window->getFrameSurface());
    }

    Rect rect = window->getContentRect();
    rect.x += windowRect.x;
    rect.y += windowRect.y;

    data->mutex->lock();
    if (data->frameTexture != nullptr)
    {
        data->frameTexture->draw(windowRect, m_scale);
    }
    if (data->contentTexture != nullptr)
    {
        data->contentTexture->draw(rect, m_scale);
    }
    data->mutex->unlock();

    return true;
}

void OpenGLDisplay::endDraw()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();

    glFlush();

    swapBuffers();

    releaseCurrentContext();
}

void OpenGLDisplay::update(Window* window, Geek::Gfx::Surface* surface)
{
    OpenGLWindowDisplayData* data = getData(window);
    setCurrentContext();
    updateTexture(data, data->contentTexture, surface);
    releaseCurrentContext();
}

void OpenGLDisplay::updateFrame(Window* window)
{
    Surface* frameSurface = window->getFrameSurface();
    if (frameSurface == nullptr)
    {
        return;
    }
    setCurrentContext();
    OpenGLWindowDisplayData* data = getData(window);
    if (data->frameTexture == nullptr)
    {
        data->frameTexture = new OpenGLTexture();
    }
    updateTexture(data, data->frameTexture, window->getFrameSurface());
    releaseCurrentContext();
}

OpenGLWindowDisplayData* OpenGLDisplay::getData(Window* window)
{
    auto data = (OpenGLWindowDisplayData*)window->getWindowDisplayData(this);
    if (data == nullptr)
    {
        data = new OpenGLWindowDisplayData();
        data->mutex = Thread::createMutex();
        data->contentTexture = new OpenGLTexture();
        window->setWindowDisplayData(this, data);
    }

    return data;
}

void OpenGLDisplay::updateTexture(OpenGLWindowDisplayData* data, OpenGLTexture* texture, Geek::Gfx::Surface* surface)
{
    data->mutex->lock();

    texture->update(surface);

    data->mutex->unlock();
}

void OpenGLDisplay::drawCursor(Cursor* cursor, Geek::Vector2D pos)
{
    glEnable(GL_BLEND);
    if (m_cursorTexture == nullptr)
    {
        m_cursorTexture = new OpenGLTexture();
    }
    if (m_currentCursor != cursor)
    {
        m_cursorTexture->update(cursor->getSurface());
        m_currentCursor = cursor;
    }

    Rect rect;
    rect.x = pos.x;
    rect.y = pos.y;
    rect.w = cursor->getSurface()->getWidth() / m_scale;
    rect.h = cursor->getSurface()->getHeight() / m_scale;
    m_cursorTexture->draw(rect, m_scale);
    glDisable(GL_BLEND);
}

OpenGLTexture::OpenGLTexture()
{
    glGenTextures(1, &texture);
}

OpenGLTexture::~OpenGLTexture()
{
    delete surface;
}

void OpenGLTexture::update(Geek::Gfx::Surface* newSurface)
{
    int width = newSurface->getWidth();
    int height = newSurface->getHeight();

    unsigned int textureWidth = powerOfTwo(width * 1);
    unsigned int textureHeight = powerOfTwo(height * 1);

    if (surface == nullptr || surface->getWidth() != textureWidth || surface->getHeight() != textureHeight)
    {
        delete surface;

        surface = new Surface(textureWidth, textureHeight, 4);
        surface->clear(0xff000000);
    }

    coordX = (float) (width * 1) / (float) textureWidth;
    coordY = (float) (height * 1) / (float) textureHeight;

    surface->blit(0, 0, newSurface, true);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        textureWidth,
        textureHeight,
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        surface->getData());
}

void OpenGLTexture::draw(Rect rect, float scale)
{
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, texture);
    glBegin(GL_TRIANGLE_STRIP);

    rect.x *= scale;
    rect.y *= scale;
    rect.w *= scale;
    rect.h *= scale;

    glTexCoord2f(0, 0);
    glVertex2i(rect.x, rect.y);

    glTexCoord2f(coordX, 0);
    glVertex2i(rect.x + (rect.w), rect.y);

    glTexCoord2f(0, coordY);
    glVertex2i(rect.x, rect.y + (rect.h));

    glTexCoord2f(coordX, coordY);
    glVertex2i(rect.x + (rect.w), rect.y + (rect.h));

    glEnd();
}

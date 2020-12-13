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
    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    return true;
}

bool OpenGLDisplay::startDraw()
{
    setCurrentContext();

    glMatrixMode(GL_MODELVIEW);
    // clear the drawing buffer.
    glClear(GL_COLOR_BUFFER_BIT);
    // clear the identity matrix.
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    /* This allows alpha blending of 2D textures with the scene */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);

    float scale = 2.0;

    glViewport(0, 0, m_rect.w * scale, m_rect.h * scale);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, (double)m_rect.w * scale, (double)m_rect.h * scale, 0.0, 0.0, 1.0);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    return true;
}

bool OpenGLDisplay::draw(Window* window, Geek::Rect drawRect)
{
    OpenGLWindowDisplayData* data = getData(window);
    Rect windowRect = window->getRect();

    if (!data->frameTexture.valid && window->hasFrame())
    {
        updateTexture(window, data, &(data->frameTexture), window->getFrameSurface());
    }

    Rect rect = window->getContentRect();
    rect.x += windowRect.x;
    rect.y += windowRect.y;

    data->mutex->lock();
    if (data->frameTexture.valid)
    {
        drawTexture(&(data->frameTexture), windowRect);
    }
    if (data->contentTexture.valid)
    {
        drawTexture(&(data->contentTexture), rect);
    }
    data->mutex->unlock();

    return true;
}

void OpenGLDisplay::drawTexture(const OpenGLWindowTexture* texture, Rect rect)
{
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, texture->texture);
    glBegin(GL_TRIANGLE_STRIP);

    float scale = 2.0f;
    rect.x *= scale;
    rect.y *= scale;
    rect.w *= scale;
    rect.h *= scale;

    glTexCoord2f(0, 0);
    glVertex2i(rect.x, rect.y);

    glTexCoord2f(texture->coordX, 0);
    glVertex2i(rect.x + (rect.w), rect.y);

    glTexCoord2f(0, texture->coordY);
    glVertex2i(rect.x, rect.y + (rect.h));

    glTexCoord2f(texture->coordX, texture->coordY);
    glVertex2i(rect.x + (rect.w), rect.y + (rect.h));

    glEnd();
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
}

void OpenGLDisplay::update(Window* window, Geek::Gfx::Surface* surface)
{
    OpenGLWindowDisplayData* data = getData(window);
    updateTexture(window, data, &(data->contentTexture), surface);
}

void OpenGLDisplay::updateFrame(Window* window)
{
    if (window->getFrameSurface() == nullptr)
    {
        return;
    }
    OpenGLWindowDisplayData* data = getData(window);
    updateTexture(window, data, &(data->frameTexture), window->getFrameSurface());
}

OpenGLWindowDisplayData* OpenGLDisplay::getData(Window* window)
{
    auto data = (OpenGLWindowDisplayData*)window->getWindowDisplayData(this);
    if (data == nullptr)
    {
        data = new OpenGLWindowDisplayData();
        data->mutex = Thread::createMutex();
        window->setWindowDisplayData(this, data);
    }

    return data;
}

void OpenGLDisplay::updateTexture(OpenGLWindowDisplayData* data, OpenGLWindowTexture* texture, Geek::Gfx::Surface* surface)
{
    data->mutex->lock();

    int winWidth = surface->getWidth();
    int winHeight = surface->getHeight();

    int scale = 1;

    unsigned int textureWidth = powerOfTwo(winWidth * scale);
    unsigned int textureHeight = powerOfTwo(winHeight * scale);
    texture->coordX = (float)(winWidth * scale) / (float)textureWidth;
    texture->coordY = (float)(winHeight * scale) / (float)textureHeight;

    if (texture->surface == nullptr ||
        texture->surface->getWidth() != textureWidth ||
        texture->surface->getHeight() != textureHeight )
    {
        if (texture->surface != nullptr)
        {
            delete texture->surface;
        }
        texture->surface = new Surface(textureWidth, textureHeight, 4);
        texture->surface->clear(0);
    }

    texture->surface->blit(0, 0, surface);

    setCurrentContext();

    if (texture->texture == 0)
    {
        glGenTextures(1, &(texture->texture));
    }

    glBindTexture(GL_TEXTURE_2D, texture->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        texture->surface->getWidth(), texture->surface->getHeight(),
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        texture->surface->getData());

    texture->valid = true;
    data->mutex->unlock();
}


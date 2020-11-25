//
// Created by Ian Parker on 05/11/2020.
//

#include "sdl.h"
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

SDLDisplayDriver::SDLDisplayDriver(DisplayServer* displayServer) : DisplayDriver("SDL", displayServer)
{
}

SDLDisplayDriver::~SDLDisplayDriver()
{
}

bool SDLDisplayDriver::init()
{
    int res;
    res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (res < 0)
    {
        log(ERROR, "init: Failed to initialise SDL!");
        return false;
    }

    SDLDisplay* display = new SDLDisplay(this);
    display->init();

    m_displays.insert(make_pair(display->getWindow(), display));
    m_displayServer->addDisplay(display);

    return true;
}

bool SDLDisplayDriver::poll()
{
    SDL_Event event;
    log(DEBUG, "poll: Waiting for events...");
    while (SDL_WaitEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                log(INFO, "checkEvent: SDL_QUIT!");
                return false;

            case SDL_MOUSEMOTION:
            {
                SDL_Window* sdlWindow = SDL_GetWindowFromID(event.button.windowID);
                auto it = m_displays.find(sdlWindow);
                if (it != m_displays.end())
                {
                    SDLDisplay* display = it->second;
                    Event* mouseEvent = new Event();
                    mouseEvent->eventType = AWESOME_EVENT_MOUSE_MOTION;
                    mouseEvent->mouse.x = event.button.x + display->getRect().x;
                    mouseEvent->mouse.y = event.button.y + display->getRect().y;

                    m_displayServer->getCompositor()->postEvent(mouseEvent);
                }
            } break;

            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
            {
                SDL_Window* sdlWindow = SDL_GetWindowFromID(event.button.windowID);
                auto it = m_displays.find(sdlWindow);
                if (it != m_displays.end())
                {
                    SDLDisplay* display = it->second;
                    Event* mouseEvent = new Event();
                    mouseEvent->eventType = AWESOME_EVENT_MOUSE_BUTTON;
                    mouseEvent->mouse.x = event.button.x + display->getRect().x;
                    mouseEvent->mouse.y = event.button.y + display->getRect().y;
                    mouseEvent->mouse.button.buttons = 1;
                    mouseEvent->mouse.button.direction = (event.type == SDL_MOUSEBUTTONDOWN);

                    m_displayServer->getCompositor()->postEvent(mouseEvent);
                }
            } break;

            case SDL_WINDOWEVENT:
            {
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_CLOSE:
                        log(INFO, "checkEvent: SDL_WINDOWEVENT_CLOSE!");
                        return false;
                }
            } break;

        }
    }

    return true;
}

SDLDisplay::SDLDisplay(SDLDisplayDriver* displayDriver) : Display("SDLDisplay", displayDriver)
{
}

SDLDisplay::~SDLDisplay()
{

}

bool SDLDisplay::init()
{
    m_rect.w = 1024;
    m_rect.h = 768;

    int flags =
        SDL_WINDOW_OPENGL |
        SDL_WINDOW_SHOWN |
        SDL_WINDOW_ALLOW_HIGHDPI ;

    log(DEBUG, "init: Creating window...");
    m_window = SDL_CreateWindow(
        "Awesome SDL Display",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        m_rect.w,
        m_rect.h,
        flags);
    log(DEBUG, "init: m_window=%p", m_window);
    if (m_window == NULL)
    {
        log( ERROR, "open: Failed to open window: %s", SDL_GetError());
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);

    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    return true;
}

bool SDLDisplay::startDraw()
{
    SDL_GL_MakeCurrent(m_window, m_glContext);

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

bool SDLDisplay::draw(Window* window, Geek::Rect drawRect)
{
    SDLWindowDisplayData* data = getData(window);
    Rect windowRect = window->getRect();

    if (!data->valid)
    {
        log(DEBUG, "draw: Creating default surface");
        Surface* surface = new Surface(windowRect.w, windowRect.h, 4);
        surface->clear(0);
        updateTexture(window, data, surface);
        delete surface;
    }

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

    log(DEBUG, "draw: Drawing window...");
    data->mutex->lock();
    glBindTexture(GL_TEXTURE_2D, data->texture);
    glBegin(GL_TRIANGLE_STRIP);

    float scale = 2.0f;

    glTexCoord2f(0, 0);
    glVertex2i(windowRect.x, windowRect.y);

    glTexCoord2f(data->textureCoordX, 0);
    glVertex2i(windowRect.x + (windowRect.w * scale), windowRect.y);

    glTexCoord2f(0, data->textureCoordY);
    glVertex2i(windowRect.x, windowRect.y + (windowRect.h * scale));

    glTexCoord2f(data->textureCoordX, data->textureCoordY);
    glVertex2i(windowRect.x + (windowRect.w * scale), windowRect.y + (windowRect.h * scale));

    glEnd();
    data->mutex->unlock();
    log(DEBUG, "draw: Done!");

    return true;
}

void SDLDisplay::endDraw()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();

    glFlush();

    SDL_GL_SwapWindow(m_window);
}

void SDLDisplay::update(Window* window, Geek::Gfx::Surface* surface)
{
    updateTexture(window, getData(window), surface);
}

SDLWindowDisplayData* SDLDisplay::getData(Window* window)
{
    auto data = (SDLWindowDisplayData*)window->getWindowDisplayData(this);
    if (data == nullptr)
    {
        data = new SDLWindowDisplayData();
        data->mutex = Thread::createMutex();
        window->setWindowDisplayData(this, data);
    }

    return data;
}

void SDLDisplay::updateTexture(Window* window, SDLWindowDisplayData* data, Geek::Gfx::Surface* surface)
{
    data->mutex->lock();

    log(DEBUG, "updateTexture: window=%p, data=%p", window, data);
    int winWidth = window->getRect().w;
    int winHeight = window->getRect().h;

    float scale = 2;

    unsigned int textureWidth = powerOfTwo(winWidth * scale);
    unsigned int textureHeight = powerOfTwo(winHeight * scale);
    data->textureCoordX = (float)(winWidth * scale) / (float)textureWidth;
    data->textureCoordY = (float)(winHeight * scale) / (float)textureHeight;

    if (data->textureSurface == nullptr ||
        data->textureSurface->getWidth() != textureWidth ||
        data->textureSurface->getHeight() != textureHeight )
    {
        if (data->textureSurface != nullptr)
        {
            delete data->textureSurface;
        }
        data->textureSurface = new Surface(textureWidth, textureHeight, 4);
        data->textureSurface->clear(0);
    }

    data->textureSurface->blit(0, 0, surface);

    SDL_GL_MakeCurrent(m_window, m_glContext);

    if (data->texture == 0)
    {
        glGenTextures(1, &(data->texture));
    }

    glBindTexture(GL_TEXTURE_2D, data->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        data->textureSurface->getWidth(), data->textureSurface->getHeight(),
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        data->textureSurface->getData());

    data->valid = true;
    data->mutex->unlock();
}

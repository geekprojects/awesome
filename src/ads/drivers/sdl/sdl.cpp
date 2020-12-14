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

SDLDisplayDriver::SDLDisplayDriver(DisplayServer* displayServer) : OpenGLDisplayDriver("SDL", displayServer)
{
}

SDLDisplayDriver::~SDLDisplayDriver()
{
    SDL_Quit();
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

SDLDisplay::SDLDisplay(SDLDisplayDriver* displayDriver) : OpenGLDisplay("SDLDisplay", displayDriver)
{
}

SDLDisplay::~SDLDisplay()
{
    if (m_glContext != nullptr)
    {
        SDL_GL_DeleteContext(m_glContext);
    }
    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
    }
}

bool SDLDisplay::init()
{
    m_rect.w = 1024;
    m_rect.h = 768;

    unsigned int flags =
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
    if (m_window == nullptr)
    {
        log( ERROR, "open: Failed to open window: %s", SDL_GetError());
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);

    OpenGLDisplay::init();

    return true;
}

void SDLDisplay::setCurrentContext()
{
    SDL_GL_MakeCurrent(m_window, m_glContext);
}

void SDLDisplay::releaseCurrentContext()
{
}

void SDLDisplay::swapBuffers()
{
    SDL_GL_SwapWindow(m_window);
}


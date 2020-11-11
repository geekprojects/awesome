//
// Created by Ian Parker on 05/11/2020.
//

#include <awesome/displaydrivers/sdl.h>
#include <awesome/displayserver.h>

using namespace Awesome;
using namespace Geek;

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
    m_displayServer->addDisplay(display);

    return true;
}

bool SDLDisplayDriver::poll()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                log(INFO, "checkEvent: SDL_QUIT!");
                break;
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

    return true;
}

bool SDLDisplay::startDraw()
{
    return true;
}

bool SDLDisplay::draw(Window* window, Geek::Rect drawRect)
{
    auto data = (SDLWindowDisplayData*)window->getWindowDisplayData();
    if (data == nullptr)
    {
        data = new SDLWindowDisplayData();
        window->setWindowDisplayData(data);
    }

    return true;
}

void SDLDisplay::endDraw()
{
}


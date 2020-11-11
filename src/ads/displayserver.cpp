//
// Created by Ian Parker on 05/11/2020.
//

#include <awesome/displayserver.h>
#include <awesome/displaydrivers/sdl.h>

using namespace Awesome;
using namespace Geek;

DisplayServer::DisplayServer()
{
    m_messageSignal = Thread::createCondVar();
}

DisplayServer::~DisplayServer()
{

}

bool DisplayServer::init()
{
    bool res;

    m_compositor = new Compositor(this);

    SDLDisplayDriver* openGlDisplayDriver = new SDLDisplayDriver(this);
    res = openGlDisplayDriver->init();
    if (res)
    {
        m_displayDrivers.push_back(openGlDisplayDriver);
    }

    return true;
}

void DisplayServer::main()
{
    while (m_running)
    {
        for (DisplayDriver* driver : m_displayDrivers)
        {
            driver->poll();
        }

        for (Display* display : m_displays)
        {
            m_compositor->draw(display);
        }

        m_messageSignal->wait(100);
    }
}

void DisplayServer::addDisplay(Display* display)
{
    m_displays.push_back(display);
}


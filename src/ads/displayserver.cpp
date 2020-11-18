//
// Created by Ian Parker on 05/11/2020.
//

#include <awesome/displayserver.h>
#include "drivers/sdl/sdl.h"
#include "interfaces/awesome/awesome.h"

using namespace Awesome;
using namespace Geek;
using namespace std;

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

    AwesomeInterface* awesomeInterface = new AwesomeInterface(this);
    awesomeInterface->init();

    return true;
}

void DisplayServer::main()
{
    while (m_running)
    {
        for (DisplayDriver* driver : m_displayDrivers)
        {
            bool res;
            res = driver->poll();
            if (!res)
            {
                m_running = false;
            }
        }

        for (Display* display : m_displays)
        {
            m_compositor->draw(display);
        }

        m_messageSignal->wait(1000);
    }
}

void DisplayServer::addDisplay(Display* display)
{
    m_displays.push_back(display);
}

void DisplayServer::addClient(Client* client)
{
    m_clients.push_back(client);
}

void DisplayServer::removeClient(Client* client)
{
    // Remove all windows
    vector<Window*> removeWindows;
    for (Window* window : m_compositor->getWindows())
    {
        if (window->getClient() == client)
        {
            removeWindows.push_back(window);
        }
    }
    for (Window* window : removeWindows)
    {
        m_compositor->removeWindow(window);
    }

}


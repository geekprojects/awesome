//
// Created by Ian Parker on 05/11/2020.
//

#include <awesome/displayserver.h>
#include "drivers/sdl/sdl.h"
#include "interfaces/awesome/awesome.h"

using namespace Awesome;
using namespace Geek;
using namespace std;

DisplayServer::DisplayServer() : Logger("DisplayServer")
{
    m_messageSignal = Thread::createCondVar();
    m_drawSignal = Thread::createCondVar();
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

    m_drawThread = new DisplayServerDrawThread(this);

    return true;
}

void DisplayServer::main()
{
    m_running = true;

    m_drawThread->start();

    while (m_running)
    {
        log(DEBUG, "main: Polling...");
        for (DisplayDriver* driver : m_displayDrivers)
        {
            bool res;
            res = driver->poll();
            if (!res)
            {
                m_running = false;
            }
        }
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

DisplayServerDrawThread::DisplayServerDrawThread(DisplayServer* displayServer)
{
    m_displayServer = displayServer;
}

DisplayServerDrawThread::~DisplayServerDrawThread()
{

}

bool DisplayServerDrawThread::main()
{
    while (m_displayServer->isRunning())
    {
        for (Display* display : m_displayServer->getDisplays())
        {
            m_displayServer->getCompositor()->draw(display);
        }

        m_displayServer->getDrawSignal()->wait();
    }

    return true;
}


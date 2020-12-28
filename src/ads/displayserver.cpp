#include <awesome/displayserver.h>
#ifdef AWESOME_ENGINE_DRM
#include "drivers/linuxdrm/linuxdrm.h"
#endif
#ifdef AWESOME_ENGINE_SDL
#include "drivers/sdl/sdl.h"
#endif
#include "interfaces/awesome/awesome.h"

#include <unistd.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

DisplayServer::DisplayServer() : Logger("DisplayServer")
{
    m_drawSignal = Thread::createCondVar();
}

DisplayServer::~DisplayServer()
{
    for (Interface* iface : m_interfaces)
    {
        delete iface;
    }

    for (DisplayDriver* driver : m_displayDrivers)
    {
        delete driver;
    }

    delete m_compositor;

    delete m_fontManager;
}

bool DisplayServer::init()
{
    bool res;

    m_fontManager = new FontManager();
    m_fontManager->init();
#if defined(__APPLE__) && defined(__MACH__)
    m_fontManager->scan("/Library/Fonts");
    m_fontManager->scan("/System/Library/Fonts");
    const char* homechar = getenv("HOME");
    m_fontManager->scan(string(homechar) + "/Library/Fonts");
#else
    m_fontManager->scan("/usr/share/fonts");
#endif

    m_compositor = new Compositor(this);
    res = m_compositor->init();
    if (!res)
    {
        return false;
    }

#ifdef AWESOME_ENGINE_DRM
    DisplayDriver* displayDriver = new DRMDisplayDriver(this);
#elif defined(AWESOME_ENGINE_SDL)
    DisplayDriver* displayDriver = new SDLDisplayDriver(this);
#endif
    res = displayDriver->init();
    if (res)
    {
        m_displayDrivers.push_back(displayDriver);
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

    // Tell the draw thread to stop
    m_drawSignal->signal();
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

    m_drawSignal->signal();
}

void DisplayServer::quit()
{
    for (DisplayDriver* driver : m_displayDrivers)
    {
        driver->quit();
    }
}

Display* DisplayServer::getDisplayAt(Geek::Vector2D pos) const
{
    for (Display* display : m_displays)
    {
        if (display->getRect().contains(pos))
        {
            return display;
        }
    }
    return nullptr;
}

DisplayServerDrawThread::DisplayServerDrawThread(DisplayServer* displayServer)
{
    m_displayServer = displayServer;
}

DisplayServerDrawThread::~DisplayServerDrawThread() = default;

bool DisplayServerDrawThread::main()
{
    while (m_displayServer->isRunning())
    {
        for (Display* display : m_displayServer->getDisplays())
        {
            m_displayServer->getCompositor()->draw(display);
        }

        usleep(1000000 / 100);

        m_displayServer->getDrawSignal()->wait();
    }

    return true;
}


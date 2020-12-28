//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_DISPLAYSERVER_H
#define AWESOME_DISPLAYSERVER_H

#include <awesome/interface.h>
#include <awesome/displaydriver.h>
#include <awesome/display.h>
#include <awesome/client.h>
#include <awesome/compositor.h>

#include <geek/core-thread.h>
#include <geek/fonts.h>

#include <vector>

namespace Awesome
{
class DisplayServerDrawThread;

class DisplayServer : Geek::Logger
{
 private:
    std::vector<Interface*> m_interfaces;
    std::vector<DisplayDriver*> m_displayDrivers;
    std::vector<Display*> m_displays;
    std::vector<Client*> m_clients;

    Compositor* m_compositor = nullptr;

    DisplayServerDrawThread* m_drawThread = nullptr;
    Geek::FontManager* m_fontManager = nullptr;

    Geek::CondVar* m_drawSignal;
    bool m_running = true;

 public:
    DisplayServer();
    ~DisplayServer();

    bool init();
    void main();
    void quit();

    void addDisplay(Display* display);

    const std::vector<Display*> &getDisplays() const
    {
        return m_displays;
    }

    void addClient(Client* client);
    void removeClient(Client* client);

    bool isRunning() const
    {
        return m_running;
    }

    Compositor* getCompositor() const
    {
        return m_compositor;
    }

    Geek::CondVar* getDrawSignal() const
    {
        return m_drawSignal;
    }

    Geek::FontManager* getFontManager() const
    {
        return m_fontManager;
    }

    Display* getDisplayAt(Geek::Vector2D pos) const;
};

class DisplayServerDrawThread : public Geek::Thread
{
 private:
    DisplayServer* m_displayServer;

 public:
    explicit DisplayServerDrawThread(DisplayServer* displayServer);
    ~DisplayServerDrawThread();

    bool main() override;

};

}

#endif //AWESOME_DISPLAYSERVER_H

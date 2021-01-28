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

#include <yaml-cpp/yaml.h>

#include <vector>

namespace Awesome
{
class DisplayServerDrawThread;

class DisplayServer : Geek::Logger
{
 private:
    YAML::Node m_config;

    std::vector<Interface*> m_interfaces;
    std::vector<DisplayDriver*> m_displayDrivers;
    std::vector<Display*> m_displays;
    std::vector<Client*> m_clients;

    int m_totalWidth = 0;
    int m_totalHeight = 0;

    Compositor* m_compositor = nullptr;

    DisplayServerDrawThread* m_drawThread = nullptr;
    Geek::FontManager* m_fontManager = nullptr;

    Geek::Gfx::Surface* m_closeButtonActive = nullptr;
    Geek::Gfx::Surface* m_closeButtonInactive = nullptr;
    Geek::Gfx::Surface* m_closeButtonHover = nullptr;

 private:

    Geek::CondVar* m_drawSignal;
    bool m_running = true;

    DisplayDriver* createDisplayDriver();

 public:
    DisplayServer();
    ~DisplayServer();

    bool init();
    void main();
    void quit();

    void addDisplay(Display* display);

    bool loadConfig(std::string configFile);

    const YAML::Node& getConfig() const
    {
        return m_config;
    }

    [[nodiscard]] const std::vector<Display*> &getDisplays() const
    {
        return m_displays;
    }

    void addClient(Client* client);
    void removeClient(Client* client);

    bool isRunning() const
    {
        return m_running;
    }

    [[nodiscard]] Compositor* getCompositor() const
    {
        return m_compositor;
    }

    [[nodiscard]] Geek::CondVar* getDrawSignal() const
    {
        return m_drawSignal;
    }

    Geek::FontManager* getFontManager() const
    {
        return m_fontManager;
    }

    Display* getDisplayAt(Geek::Vector2D pos) const;

    int getTotalWidth() const
    {
        return m_totalWidth;
    }

    int getTotalHeight() const
    {
        return m_totalHeight;
    }

    Geek::Gfx::Surface* getCloseButtonInactive() const
    {
        return m_closeButtonInactive;
    }
    Geek::Gfx::Surface* getCloseButtonHover() const
    {
        return m_closeButtonHover;
    }
    Geek::Gfx::Surface* getCloseButtonActive() const
    {
        return m_closeButtonActive;
    }
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

//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_DISPLAYSERVER_H
#define AWESOME_DISPLAYSERVER_H

#include <awesome/interface.h>
#include <awesome/displaydriver.h>
#include <awesome/display.h>
#include <awesome/compositor.h>

#include <geek/core-thread.h>

#include <vector>

namespace Awesome
{

class DisplayServer
{
 private:
    std::vector<Interface*> m_interfaces;
    std::vector<DisplayDriver*> m_displayDrivers;
    std::vector<Display*> m_displays;
    Compositor* m_compositor;
 public:
 private:

    Geek::CondVar* m_messageSignal;
    bool m_running = true;

 public:
    DisplayServer();
    ~DisplayServer();

    bool init();
    void main();

    void addDisplay(Display* display);

    Compositor* getCompositor() const
    {
        return m_compositor;
    }
};

}

#endif //AWESOME_DISPLAYSERVER_H

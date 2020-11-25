
#include "awesome/compositor.h"
#include <awesome/displayserver.h>

using namespace Awesome;
using namespace Geek;

Compositor::Compositor(DisplayServer* displayServer) : Logger("Compositor")
{
    m_displayServer = displayServer;
}

Compositor::~Compositor()
{

}

void Compositor::addWindow(Window* window)
{
    Rect windowRect = window->getRect();

    window->setId(m_windowIdx++);
    m_windows.push_back(window);

    for (Display* display : m_displayServer->getDisplays())
    {
        Rect displayRect = display->getRect();
        if (displayRect.intersects(windowRect))
        {
            window->setWindowDisplayData(display, nullptr);
        }
    }
}

void Compositor::draw(Display* display)
{
    Rect displayRect = display->getRect();

    bool res;
    res = display->startDraw();
    if (!res)
    {
        // Display isn't currently available
        return;
    }

    for (Window* window : m_windows)
    {
        Rect windowRect = window->getRect();
        if (!displayRect.intersects(windowRect))
        {
            continue;
        }

        Rect visibleRect = windowRect.clipCopy(displayRect);
        log(DEBUG, "draw:  -> visibleRect=%s", visibleRect.toString().c_str());
        display->draw(window, visibleRect);
    }
    display->endDraw();
}

void Compositor::update(Window* window)
{

}

void Compositor::removeWindow(Window* window)
{
    for (auto it = m_windows.begin(); it != m_windows.end(); it++)
    {
        if (*it == window)
        {
            m_windows.erase(it);
            break;
        }
    }
}

Window* Compositor::findWindow(int id)
{
    for (Window* window : m_windows)
    {
        if (window->getId() == id)
        {
            return window;
        }
    }
    return nullptr;
}

void Compositor::postEvent(Event* event)
{
    bool posted = false;
    switch (event->getCategory())
    {
        case AWESOME_EVENT_MOUSE:
        {
            Vector2D mousePos(event->mouse.x, event->mouse.y);
            Window* targetWindow = nullptr;
            for (Window* window : m_windows)
            {
                if (window->getRect().contains(mousePos))
                {
                    targetWindow = window;
                    break;
                }
            }
            if (targetWindow != nullptr)
            {
                event->windowId = targetWindow->getId();
                event->mouse.x -= targetWindow->getRect().x;
                event->mouse.y -= targetWindow->getRect().y;
                targetWindow->postEvent(event);
                posted = true;
            }
        } break;
    }

    if (!posted)
    {
        delete event;
    }
}



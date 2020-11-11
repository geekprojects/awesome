
#include <awesome/compositor.h>
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
    m_windows.push_back(window);
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
        log(DEBUG, "draw: Window: %p", window);
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


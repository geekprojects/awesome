
#include "awesome/compositor.h"
#include <awesome/displayserver.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

Compositor::Compositor(DisplayServer* displayServer) : Logger("Compositor")
{
    m_displayServer = displayServer;
    m_windowMutex = Thread::createMutex();
}

Compositor::~Compositor() = default;

bool Compositor::init()
{
    m_cursor = Cursor::loadCursor();

    return true;
}

void Compositor::addWindow(Window* window)
{
    Rect windowRect = window->getRect();

    Vector2D pos = window->getPosition();
    if (pos.x == WINDOW_POSITION_ANY || pos.y == WINDOW_POSITION_ANY)
    {
        if (pos.x == WINDOW_POSITION_ANY)
        {
            pos.x = 50;
        }
        if (pos.y == WINDOW_POSITION_ANY)
        {
            pos.y = 50;
        }
        window->setPosition(pos);
    }

    m_windowMutex->lock();
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
    m_windowMutex->unlock();

    bringToFront(window);
    m_displayServer->getDrawSignal()->signal();
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

    m_windowMutex->lock();
    deque<Window*>::reverse_iterator it;
    for (it = m_backgroundWindowOrder.rbegin(); it != m_backgroundWindowOrder.rend(); it++)
    {
        drawWindow(display, displayRect, *it);
    }
    for (it = m_windowOrder.rbegin(); it != m_windowOrder.rend(); it++)
    {
        drawWindow(display, displayRect, *it);
    }
    for (it = m_foregroundWindowOrder.rbegin(); it != m_foregroundWindowOrder.rend(); it++)
    {
        drawWindow(display, displayRect, *it);
    }
    m_windowMutex->unlock();

    if (display->getRect().contains(m_mousePos))
    {
        display->drawCursor(m_cursor, m_mousePos);
    }

    display->endDraw();
}

void Compositor::drawWindow(Display* display, const Rect &displayRect, Window* window)
{
    if (!window->isVisible())
    {
        return;
    }

    Rect windowRect = window->getRect();
    if (displayRect.intersects(windowRect))
    {
        Rect visibleRect = windowRect.clipCopy(displayRect);
        display->draw(window, visibleRect);
    }
}

static void removeWindow(deque<Window*>& order, Window* window)
{
    for (auto it = order.begin(); it != order.end(); it++)
    {
        if (*it == window)
        {
            order.erase(it);
            break;
        }
    }
}

void Compositor::removeWindow(Window* window)
{
    m_windowMutex->lock();
    for (auto it = m_windows.begin(); it != m_windows.end(); it++)
    {
        if (*it == window)
        {
            m_windows.erase(it);
            break;
        }
    }
    if (window->getFlags() & WINDOW_FOREGROUND)
    {
        ::removeWindow(m_foregroundWindowOrder, window);
    }
    else if (window->getFlags() & WINDOW_BACKGROUND)
    {
        ::removeWindow(m_backgroundWindowOrder, window);
    }
    else
    {
        ::removeWindow(m_windowOrder, window);
    }
    m_windowMutex->unlock();
}

Window* Compositor::findWindow(int id)
{
    Window* result = nullptr;

    m_windowMutex->lock();
    for (Window* window : m_windows)
    {
        if (window->getId() == id)
        {
            result = window;
            break;
        }
    }
    m_windowMutex->unlock();
    return result;
}

static void bringToFront(deque<Window*>& order, Window* window)
{
    if (!order.empty() && order.front() == window)
    {
        // This window is already at the front!
        return;
    }

    for (auto it = order.begin(); it != order.end(); it++)
    {
        if (*it == window)
        {
            order.erase(it);
            break;
        }
    }
    order.push_front(window);
}

void Compositor::bringToFront(Window* window)
{
    m_windowMutex->lock();
    unsigned int flags = window->getFlags();
    if (flags & WINDOW_FOREGROUND)
    {
        ::bringToFront(m_foregroundWindowOrder, window);
    }
    else if (flags & WINDOW_BACKGROUND)
    {
        ::bringToFront(m_backgroundWindowOrder, window);
    }
    else
    {
        ::bringToFront(m_windowOrder, window);
    }
    m_windowMutex->unlock();
}

void Compositor::postEvent(Event* event)
{
    bool posted = false;
    switch (event->getCategory())
    {
        case AWESOME_EVENT_MOUSE:
        {
            Vector2D oldPos = m_mousePos;
            bool redraw = false;
            m_mousePos.set(event->mouse.x, event->mouse.y);

            if (m_dragging)
            {
                Vector2D pos = m_mousePos;
                pos -= m_draggingOffset;
                m_draggingWindow->setPosition(pos);
                if (event->eventType == AWESOME_EVENT_MOUSE_BUTTON)
                {
                    m_dragging = false;
                }
                redraw = true;
            }
            else
            {
                Window* targetWindow = findWindowAt(m_mousePos);
                if (targetWindow != nullptr)
                {
                    if (event->eventType == AWESOME_EVENT_MOUSE_BUTTON && event->mouse.button.direction)
                    {
                        bringToFront(targetWindow);
                        m_displayServer->getDrawSignal()->signal();
                    }

                    if (m_activeWindow != targetWindow)
                    {
                        if (m_activeWindow != nullptr)
                        {
                            // Lose focus
                        }
                        m_activeWindow = targetWindow;
                    }

                    bool skip = false;
                    if (event->eventType == AWESOME_EVENT_MOUSE_MOTION)
                    {
                        skip = !targetWindow->wantsMotionEvents();
                    }

                    if (event->eventType == AWESOME_EVENT_MOUSE_BUTTON)
                    {
                        log(DEBUG, "postEvent: Button: window=%d: %ls", targetWindow->getId(), targetWindow->getTitle().c_str());
                    }

                    if (!skip)
                    {
                        event->windowId = targetWindow->getId();
                        event->mouse.x -= targetWindow->getRect().x;
                        event->mouse.y -= targetWindow->getRect().y;
                        targetWindow->postEvent(event);
                        posted = true;
                    }
                    else
                    {
                        //log(DEBUG, "postEvent: Skipped");
                    }
                }
            }
            if (redraw || m_mousePos.x != oldPos.x || m_mousePos.y != oldPos.y)
            {
                m_displayServer->getDrawSignal()->signal();
            }
        } break;

        case AWESOME_EVENT_KEYBOARD:
            log(DEBUG, "postEvent: Keyboard: key=%d, modifier=0x%x, activeWindow=%p", event->key.key, event->key.modifiers, m_activeWindow);

            // TODO: Handle global key sequences

            if (m_activeWindow != nullptr)
            {
                event->windowId = m_activeWindow->getId();
                m_activeWindow->postEvent(event);
                posted = true;
            }
            break;

        case AWESOME_EVENT_WINDOW:
        case AWESOME_EVENT_MASK:
            break;
    }

    if (!posted)
    {
        delete event;
    }
}

void Compositor::startDrag(Window* drag)
{
    m_dragging = true;
    m_draggingOffset = m_mousePos;
    m_draggingOffset -= drag->getPosition();
    m_draggingWindow = drag;
}

static Window* findWindowAt(const deque<Window*>& windowOrder, const Vector2D& pos)
{
    for (Window* window : windowOrder)
    {
        if (window->isVisible() && window->getRect().contains(pos))
        {
            return window;
        }
    }
    return nullptr;
}

Window* Compositor::findWindowAt(const Vector2D &pos) const
{
    Window* window;

    m_windowMutex->lock();
    window = ::findWindowAt(m_foregroundWindowOrder, pos);
    if (window == nullptr)
    {
        window = ::findWindowAt(m_windowOrder, pos);
        if (window == nullptr)
        {
            window = ::findWindowAt(m_backgroundWindowOrder, pos);
        }
    }
    m_windowMutex->unlock();

    return window;
}

void Compositor::dumpWindowOrder()
{
    m_windowMutex->lock();
    for (Window* window : m_foregroundWindowOrder)
    {
        log(DEBUG, "dumpWindowOrder: Foreground: %lu: %ls", window->getId(), window->getTitle().c_str());
    }
    for (Window* window : m_windowOrder)
    {
        log(DEBUG, "dumpWindowOrder: Normal: %lu: %ls", window->getId(), window->getTitle().c_str());
    }
    for (Window* window : m_backgroundWindowOrder)
    {
        log(DEBUG, "dumpWindowOrder: Background: %lu: %ls", window->getId(), window->getTitle().c_str());
    }
    m_windowMutex->unlock();
}

void Compositor::updateMousePos(Geek::Vector2D pos)
{
    /*
    m_mousePos = pos;
    Display* display = m_displayServer->getDisplayAt(m_mousePos);
    if (display != nullptr)
    {
        display->updateCursor(m_cursor, m_mousePos);
    }
     */
}

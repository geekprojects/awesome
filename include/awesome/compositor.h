
#ifndef AWESOME_COMPOSITOR_H
#define AWESOME_COMPOSITOR_H

#include <geek/core-logger.h>
#include <geek/core-thread.h>

#include <awesome/window.h>
#include <awesome/display.h>

#include <vector>
#include <deque>

namespace Awesome
{
class DisplayServer;

class Compositor : public Geek::Logger
{
 private:
    DisplayServer* m_displayServer;
    std::vector<Window*> m_windows;
    Geek::Mutex* m_windowMutex;
    int m_windowIdx = 1;

    std::deque<Window*> m_backgroundWindowOrder;
    std::deque<Window*> m_windowOrder;
    std::deque<Window*> m_foregroundWindowOrder;

    Geek::Vector2D m_mousePos;

    bool m_dragging = false;
    Window* m_draggingWindow = nullptr;
    Geek::Vector2D m_draggingOffset;

    void dumpWindowOrder();

 public:
    Compositor(DisplayServer* displayServer);

    ~Compositor();

    void addWindow(Window* window);

    const std::vector<Window*> &getWindows() const
    {
        return m_windows;
    }


    void draw(Display* display);

    Window* findWindow(int id);
    void removeWindow(Window* window);

    void startDrag(Window* drag);

    void bringToFront(Window* window);
    Window* findWindowAt(const Geek::Vector2D& pos) const;

    void postEvent(Event* event);

    void drawWindow(Display* display, const Geek::Rect &displayRect, Window* window);
};

}

#endif //AWESOME_COMPOSITOR_H

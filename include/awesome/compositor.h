
#ifndef AWESOME_COMPOSITOR_H
#define AWESOME_COMPOSITOR_H

#include <geek/core-logger.h>
#include <geek/core-thread.h>

#include <awesome/window.h>
#include <awesome/display.h>
#include <awesome/cursor.h>

#include <vector>
#include <deque>

namespace Awesome
{
class DisplayServer;

class Compositor : public Geek::Logger
{
 private:
    DisplayServer* m_displayServer = nullptr;
    std::vector<Window*> m_windows;
    Window* m_activeWindow = nullptr;
    Geek::Mutex* m_windowMutex = nullptr;
    int m_windowIdx = 1;

    std::deque<Window*> m_backgroundWindowOrder;
    std::deque<Window*> m_windowOrder;
    std::deque<Window*> m_foregroundWindowOrder;

    // TODO: Should these move to the DisplayServer?
    Geek::Vector2D m_mousePos;
    Cursor* m_cursor = nullptr;

    bool m_dragging = false;
    Window* m_draggingWindow = nullptr;
    Geek::Vector2D m_draggingOffset;

    void dumpWindowOrder();

 public:
    explicit Compositor(DisplayServer* displayServer);

    ~Compositor();

    bool init();

    void addWindow(Window* window);

    const std::vector<Window*> &getWindows() const
    {
        return m_windows;
    }

    void updateMousePos(Geek::Vector2D pos);

    const Geek::Vector2D &getMousePos() const
    {
        return m_mousePos;
    }

    Cursor* getCursor() const
    {
        return m_cursor;
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

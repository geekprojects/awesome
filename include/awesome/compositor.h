
#ifndef AWESOME_COMPOSITOR_H
#define AWESOME_COMPOSITOR_H

#include <geek/core-logger.h>

#include <awesome/window.h>
#include <awesome/display.h>

#include <vector>

namespace Awesome
{
class DisplayServer;

class Compositor : public Geek::Logger
{
 private:
    DisplayServer* m_displayServer;
    std::vector<Window*> m_windows;
    int m_windowIdx = 1;

 public:
    Compositor(DisplayServer* displayServer);

    ~Compositor();

    void addWindow(Window* window);

    const std::vector<Window*> &getWindows() const
    {
        return m_windows;
    }

    Window* findWindow(int id);

    void draw(Display* display);
    void update(Window* window);

    void removeWindow(Window* window);

    void postEvent(Event* event);
};

}

#endif //AWESOME_COMPOSITOR_H

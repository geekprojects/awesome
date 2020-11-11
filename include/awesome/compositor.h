
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

 public:
    Compositor(DisplayServer* displayServer);

    ~Compositor();

    void addWindow(Window* window);

    const std::vector<Window*> &getWindows() const
    {
        return m_windows;
    }

    void draw(Display* display);
};

}

#endif //AWESOME_COMPOSITOR_H

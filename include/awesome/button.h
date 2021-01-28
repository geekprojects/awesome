#ifndef AWESOME_BUTTON_H
#define AWESOME_BUTTON_H

#include <awesome/event.h>

#include <geek/core-maths.h>
#include <geek/gfx-surface.h>

namespace Awesome {

enum ButtonState
{
    BUTTON_INACTIVE,
    BUTTON_HOVER,
    BUTTON_ACTIVE,
};

class Button
{
 private:
    Geek::Rect m_rect;
    Geek::Gfx::Surface* m_inactiveSurface = nullptr;
    Geek::Gfx::Surface* m_hoverSurface = nullptr;
    Geek::Gfx::Surface* m_activeSurface = nullptr;

    ButtonState m_state = BUTTON_INACTIVE;
    bool m_dirty = true;

    void setState(ButtonState state)
    {
        if (m_state != state)
        {
            m_state = state;
            m_dirty = true;
        }
    }

 public:
    Button(
        int x,
        int y,
        Geek::Gfx::Surface* inactiveSurface,
        Geek::Gfx::Surface* hoverSurface,
        Geek::Gfx::Surface* activeSurface);
    ~Button();

    void draw(Geek::Gfx::Surface* surface);

    bool isDirty() const { return m_dirty; }
    bool contains(int x, int y) { return m_rect.contains(Geek::Vector2D(x, y)); }
    bool handleEvent(Awesome::Event* event);
};

}

#endif //AWESOME_BUTTON_H

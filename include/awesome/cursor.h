#ifndef AWESOME_CURSOR_H
#define AWESOME_CURSOR_H

#include <geek/gfx-surface.h>

namespace Awesome
{
class Cursor
{
 private:
    Geek::Vector2D m_hotSpot;
    Geek::Gfx::Surface* m_surface = nullptr;

 public:
    Cursor();
    ~Cursor();

    static Cursor* loadCursor();

    const Geek::Vector2D& getHotSpot() const { return m_hotSpot; }

    Geek::Gfx::Surface* getSurface() { return m_surface; }
};
}

#endif //AWESOME_CURSOR_H

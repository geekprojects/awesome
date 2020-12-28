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

    Geek::Gfx::Surface* getSurface() { return m_surface; }
};
}

#endif //AWESOME_CURSOR_H

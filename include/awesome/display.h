//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_DISPLAY_H
#define AWESOME_DISPLAY_H

#include <geek/core-logger.h>
#include <geek/core-maths.h>
#include <geek/gfx-surface.h>

#include <awesome/cursor.h>

namespace Awesome
{

class DisplayDriver;
class Window;

struct WindowDisplayData
{
    WindowDisplayData() {}
    virtual ~WindowDisplayData() {}
};

class Display : public Geek::Logger
{
 protected:
    std::string m_name;
    DisplayDriver* m_displayDriver;

    Geek::Rect m_rect;
    float m_scale = 1.0;

 public:
    Display(const std::string& name, DisplayDriver* driver);
    virtual ~Display();

    virtual void update(Window* window, Geek::Gfx::Surface* surface);
    virtual void updateFrame(Window* window);

    virtual bool init();
    virtual bool startDraw();
    virtual bool draw(Window* window, Geek::Rect drawRect);
    virtual void endDraw();

    virtual void drawCursor(Cursor* cursor, Geek::Vector2D pos);

    const Geek::Rect &getRect() const
    {
        return m_rect;
    }

    void setRect(const Geek::Rect &mRect)
    {
        m_rect = mRect;
    }

    float getScale() const
    {
        return m_scale;
    }

};

}

#endif //AWESOME_DISPLAY_H

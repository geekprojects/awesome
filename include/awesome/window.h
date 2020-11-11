#ifndef AWESOME_WINDOW_H
#define AWESOME_WINDOW_H

#include <geek/core-logger.h>
#include <geek/core-maths.h>

namespace Awesome
{

struct WindowDisplayData;

class Window : public Geek::Logger
{
 private:
    Geek::Rect m_rect;

    WindowDisplayData* m_windowDisplayData;

 public:
    Window();
    ~Window();

    const Geek::Vector2D getPosition() const
    {
        return Geek::Vector2D(m_rect.x, m_rect.y);
    }

    void setPosition(const Geek::Vector2D& mPosition)
    {
        m_rect.x = mPosition.x;
        m_rect.y = mPosition.y;
    }

    const Geek::Rect& getRect() const
    {
        return m_rect;
    }

    void setRect(const Geek::Rect &mRect)
    {
        m_rect = mRect;
    }

    WindowDisplayData* getWindowDisplayData() const
    {
        return m_windowDisplayData;
    }

    void setWindowDisplayData(WindowDisplayData* mWindowDisplayData)
    {
        m_windowDisplayData = mWindowDisplayData;
    }

};

}

#endif //AWESOME_WINDOW_H

//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_DISPLAY_H
#define AWESOME_DISPLAY_H

#include <geek/core-logger.h>
#include <geek/core-maths.h>
#include <frontier/utils.h>

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

 public:
    Display(std::string name, DisplayDriver* driver);
    virtual ~Display();

    virtual bool init();
    virtual bool startDraw();
    virtual bool draw(Window* window, Geek::Rect drawRect);
    virtual void endDraw();

    const Geek::Rect &getRect() const
    {
        return m_rect;
    }

    void setRect(const Geek::Rect &mRect)
    {
        m_rect = mRect;
    }

};

}

#endif //AWESOME_DISPLAY_H

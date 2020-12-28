//

#include <awesome/display.h>

using namespace Awesome;
using namespace std;

Display::Display(const string& name, DisplayDriver* displayDriver) : Logger("Display[" + name + "]")
{
    m_name = name;
    m_displayDriver = displayDriver;
}

Display::~Display() = default;

bool Display::init()
{
    return false;
}

bool Display::startDraw()
{
    return false;
}

bool Display::draw(Window* window, Geek::Rect drawRect)
{
    return false;
}

void Display::endDraw()
{
}

void Display::update(Window* window, Geek::Gfx::Surface* surface)
{
}

void Display::updateFrame(Window* window)
{
}

void Display::drawCursor(Cursor* cursor, Geek::Vector2D pos)
{
}


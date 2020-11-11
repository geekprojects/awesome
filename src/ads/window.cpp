
#include <awesome/window.h>

using namespace Awesome;
using namespace Geek;

Window::Window() : Logger("Window")
{
    m_rect.x = 0;
    m_rect.y = 0;
    m_rect.w = 0;
    m_rect.h = 0;
}

Window::~Window()
{

}

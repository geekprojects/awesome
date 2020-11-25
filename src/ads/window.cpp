
#include <awesome/window.h>
#include <awesome/display.h>

using namespace Awesome;
using namespace Geek;

Window::Window(Client* client) : Logger("Window")
{
    m_client = client;

    m_rect.x = 0;
    m_rect.y = 0;
    m_rect.w = 0;
    m_rect.h = 0;
}

Window::~Window()
{

}

void Window::update(Geek::Gfx::Surface* surface)
{
    for (auto it : m_windowDisplayData)
    {
        it.first->update(this, surface);
    }
}

void Window::postEvent(Event* event)
{
    if (m_client != nullptr)
    {
        m_client->postEvent(event);
    }
    else
    {
        delete event;
    }
}

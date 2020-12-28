
#include <awesome/window.h>
#include <awesome/display.h>
#include <awesome/displayserver.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;

Window::Window(DisplayServer* displayServer, Client* client) : Logger("Window")
{
    m_displayServer = displayServer;
    m_client = client;

    setContentSize(0, 0);
}

Window::~Window() = default;

void Window::update(Geek::Gfx::Surface* surface)
{
    for (auto it : m_windowDisplayData)
    {
        it.first->update(this, surface);
        if (hasFrame())
        {
            it.first->updateFrame(this);
        }
    }
}

void Window::postEvent(Event* event)
{
    if (event->is(AWESOME_EVENT_MOUSE))
    {
        Vector2D mousePos(event->mouse.x, event->mouse.y);
        if (m_contentRect.contains(mousePos))
        {
            //log(DEBUG, "postEvent: Content event: client=%p", m_client);
            if (m_client != nullptr)
            {
                event->mouse.x -= m_contentRect.x;
                event->mouse.y -= m_contentRect.y;
                m_client->postEvent(event);
            }
            else
            {
                //log(DEBUG, "postEvent: Content event: No Client :-(");
                delete event;
            }
        }
        else
        {
            //log(DEBUG, "postEvent: Frame event");
            if (event->eventType == AWESOME_EVENT_MOUSE_BUTTON && event->mouse.button.direction)
            {
                m_displayServer->getCompositor()->startDrag(this);
            }
        }
    }
    else
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
}

void Window::setContentSize(int width, int height)
{
    bool hasFrame = false;
    int borderSize = 0;
    int titleBarHeight = 0;
    if (m_flags & WINDOW_BORDER)
    {
        borderSize = 2;
        hasFrame = true;
    }
    if (m_flags & WINDOW_TITLE)
    {
        titleBarHeight = 20;
        hasFrame = true;
    }
    m_contentRect.x = borderSize;
    m_contentRect.y = borderSize + titleBarHeight;
    m_contentRect.w = width;
    m_contentRect.h = height;

    m_rect.w = m_contentRect.w + (borderSize * 2);
    m_rect.h = m_contentRect.h + (borderSize * 2) + titleBarHeight;

    delete m_frameSurface;

    if (hasFrame)
    {
        m_frameSurface = new Surface(m_rect.w * 2, m_rect.h * 2, 4);
        m_frameSurface->clear(0xff0000);

        updateFrame();
    }
}

void Window::updateFrame()
{
    m_frameSurface->clear(0xff221e21);

    FontHandle* handle = m_displayServer->getFontManager()->openFont("System Font", "Regular", 24);

    m_displayServer->getFontManager()->write(handle, m_frameSurface, 2, 2, L"X", 0xffffffff, true, nullptr);
    m_displayServer->getFontManager()->write(handle, m_frameSurface, 22, 2, m_title,  0xffffffff, true, nullptr);

    for (auto it : m_windowDisplayData)
    {
        it.first->updateFrame(this);
    }
}

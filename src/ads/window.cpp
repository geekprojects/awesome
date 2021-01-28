
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

    m_closeButton = new Button(
        5,
        10,
        m_displayServer->getCloseButtonInactive(),
        m_displayServer->getCloseButtonHover(),
        m_displayServer->getCloseButtonActive());
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
            bool skip = false;
            if (event->eventType == AWESOME_EVENT_MOUSE_MOTION)
            {
                skip = !wantsMotionEvents();
            }

            //log(DEBUG, "postEvent: Content event: client=%p", m_client);
            if (!skip && m_client != nullptr)
            {
                event->mouse.x -= m_contentRect.x;
                event->mouse.y -= m_contentRect.y;
                m_client->postEvent(event);
            }
            else
            {
                delete event;
            }
        }
        else
        {
            bool handled = false;
            handled |= m_closeButton->handleEvent(event);

            if (m_closeButton->isDirty())
            {
                updateFrame();
                m_displayServer->getDrawSignal()->signal();
            }

            //log(DEBUG, "postEvent: Frame event");
            if (event->eventType == AWESOME_EVENT_MOUSE_BUTTON)
            {
                if (!handled && event->mouse.button.direction)
                {
                    m_displayServer->getCompositor()->startDrag(this);
                }
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
        updateFrame();
    }
}

void Window::updateFrame()
{
    m_frameSurface->clear(0x0);
    int w = m_frameSurface->getWidth();
    int h = m_frameSurface->getHeight();
    m_frameSurface->drawGradRounded(0, 0, w, h, 10, 0xffc3c6ce, 0xffa6a9b2);

    FontHandle* handle = m_displayServer->getFontManager()->openFont("System Font", "Regular", 24);

    m_displayServer->getFontManager()->write(handle, m_frameSurface, 31, 7, m_title,  0xffbdc0c7, true, nullptr);
    m_displayServer->getFontManager()->write(handle, m_frameSurface, 30, 6, m_title,  0xff303030, true, nullptr);

    m_closeButton->draw(m_frameSurface);

    for (auto it : m_windowDisplayData)
    {
        it.first->updateFrame(this);
    }
}

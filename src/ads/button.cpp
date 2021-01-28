

#include <awesome/button.h>

using namespace Awesome;
using namespace Geek::Gfx;

Button::Button(int x, int y,
               Geek::Gfx::Surface* inactiveSurface,
               Geek::Gfx::Surface* hoverSurface,
               Geek::Gfx::Surface* activeSurface)
{
    m_rect.x = x;
    m_rect.y = y;
    m_rect.w = inactiveSurface->getWidth();
    m_rect.h = inactiveSurface->getHeight();

    m_inactiveSurface = inactiveSurface;
    m_hoverSurface = hoverSurface;
    m_activeSurface = activeSurface;
}

Button::~Button()
{

}

void Button::draw(Surface* surface)
{
    switch (m_state)
    {
        case BUTTON_INACTIVE:
            surface->blit(m_rect.x, m_rect.y, m_inactiveSurface, true);
            break;
        case BUTTON_HOVER:
            surface->blit(m_rect.x, m_rect.y, m_hoverSurface, true);
            break;
        case BUTTON_ACTIVE:
            surface->blit(m_rect.x, m_rect.y, m_activeSurface, true);
            break;
    }
    m_dirty = false;
}

bool Button::handleEvent(Awesome::Event* event)
{
    if (!event->is(EventCategory::AWESOME_EVENT_MOUSE))
    {
        return false;
    }
    if (!contains(event->mouse.x, event->mouse.y))
    {
        setState(BUTTON_INACTIVE);
        return false;
    }

    if (event->eventType == AWESOME_EVENT_MOUSE_MOTION)
    {
        printf("Button::handleEvent: Motion!\n");
        setState(BUTTON_HOVER);
        return true;
    }
    else if (event->eventType == AWESOME_EVENT_MOUSE_BUTTON)
    {
        printf("Button::handleEvent: Button!\n");
        if (event->mouse.button.direction)
        {
            setState(BUTTON_ACTIVE);
        }
        else
        {
            setState(BUTTON_HOVER);
        }
        return true;
    }
    return false;
}

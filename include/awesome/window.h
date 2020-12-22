#ifndef AWESOME_WINDOW_H
#define AWESOME_WINDOW_H

#include <awesome/client.h>
#include <awesome/protocol.h>

#include <geek/core-logger.h>
#include <geek/core-maths.h>
#include <geek/gfx-surface.h>

#include <map>

namespace Awesome
{

struct Display;
struct DisplayServer;
struct WindowDisplayData;

class Window : public Geek::Logger
{
 private:
    int m_id = 0;
    unsigned int m_flags = 0;
    bool m_visible = false;
    std::wstring m_title;

    DisplayServer* m_displayServer = nullptr;
    Client* m_client = nullptr;

    Geek::Rect m_rect;
    Geek::Rect m_contentRect;

    Geek::Gfx::Surface* m_frameSurface = nullptr;
    std::map<Display*, WindowDisplayData*> m_windowDisplayData;

 public:
    Window(DisplayServer* displayServer, Client* client);
    ~Window();

    int getId() const
    {
        return m_id;
    }

    void setId(int mId)
    {
        m_id = mId;
    }

    unsigned int getFlags() const
    {
        return m_flags;
    }

    void setFlags(unsigned int mFlags)
    {
        m_flags = mFlags;
    }

    bool hasFrame() const
    {
        return (m_flags & WINDOW_BORDER || m_flags & WINDOW_TITLE);
    }

    bool wantsMotionEvents() const
    {
        return m_flags & WINDOW_MOTION_EVENTS;
    }

    const std::wstring& getTitle() const
    {
        return m_title;
    }

    void setTitle(const std::wstring &mTitle)
    {
        m_title = mTitle;
    }

    bool isVisible() const
    {
        return m_visible;
    }

    void setVisible(bool mVisible)
    {
        m_visible = mVisible;
    }

    Client* getClient() const
    {
        return m_client;
    }

    const Geek::Vector2D getPosition() const
    {
        return Geek::Vector2D(m_rect.x, m_rect.y);
    }

    void setPosition(const Geek::Vector2D& mPosition)
    {
        m_rect.x = mPosition.x;
        m_rect.y = mPosition.y;
    }

    void setContentSize(int width, int height);

    const Geek::Rect& getContentRect() const
    {
        return m_contentRect;
    }

    const Geek::Rect& getRect() const
    {
        return m_rect;
    }

    void setRect(const Geek::Rect &mRect)
    {
        m_rect = mRect;
    }

    Geek::Gfx::Surface* getFrameSurface()
    {
        return m_frameSurface;
    }

    WindowDisplayData* getWindowDisplayData(Display* display) const
    {
        auto it = m_windowDisplayData.find(display);
        if (it != m_windowDisplayData.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void setWindowDisplayData(Display* display, WindowDisplayData* mWindowDisplayData)
    {
        m_windowDisplayData.insert_or_assign(display, mWindowDisplayData);
    }

    void update(Geek::Gfx::Surface* surface);

    void updateFrame();

    void postEvent(Event* event);
};

}

#endif //AWESOME_WINDOW_H

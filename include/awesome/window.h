#ifndef AWESOME_WINDOW_H
#define AWESOME_WINDOW_H

#include <awesome/client.h>

#include <geek/core-logger.h>
#include <geek/core-maths.h>
#include <geek/gfx-surface.h>

#include <map>

namespace Awesome
{

struct Display;
struct WindowDisplayData;

class Window : public Geek::Logger
{
 private:
    int m_id;
    Client* m_client;

    Geek::Rect m_rect;

    std::map<Display*, WindowDisplayData*> m_windowDisplayData;

 public:
    explicit Window(Client* client);
    ~Window();

    int getId() const
    {
        return m_id;
    }

    void setId(int mId)
    {
        m_id = mId;
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

    const Geek::Rect& getRect() const
    {
        return m_rect;
    }

    void setRect(const Geek::Rect &mRect)
    {
        m_rect = mRect;
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
};

}

#endif //AWESOME_WINDOW_H

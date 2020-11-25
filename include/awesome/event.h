#ifndef AWESOME_EVENT_H
#define AWESOME_EVENT_H

#include <awesome/types.h>

namespace Awesome
{

enum EventCategory
{
    AWESOME_EVENT_KEYBOARD = 0x10000,
    AWESOME_EVENT_MOUSE    = 0x20000,
    AWESOME_EVENT_WINDOW   = 0x40000,
    AWESOME_EVENT_MASK     = 0xFFFF0000
};

enum EventType
{
    AWESOME_EVENT_KEY =          AWESOME_EVENT_KEYBOARD | 0x1,
    AWESOME_EVENT_MOUSE_BUTTON = AWESOME_EVENT_MOUSE | 0x1,
    AWESOME_EVENT_MOUSE_MOTION = AWESOME_EVENT_MOUSE | 0x2,
    AWESOME_EVENT_MOUSE_SCROLL = AWESOME_EVENT_MOUSE | 0x3
};

enum MouseButton
{
    BUTTON_LEFT  = 1,
    BUTTON_RIGHT = 2
};

struct Event
{
    EventType eventType;
    WindowID windowId;

    Event() {}
    ~Event() = default;

    bool is(EventCategory category) const
    {
        return ((unsigned int) eventType & (unsigned int) AWESOME_EVENT_MASK) == (unsigned int) category;
    }

    EventCategory getCategory() const
    {
        return EventCategory(eventType & AWESOME_EVENT_MASK);
    }

    union
    {
        struct
        {
            bool direction;
            uint32_t key;
            wchar_t chr;
            uint32_t modifiers;
        } key;

        struct
        {
            uint32_t x;
            uint32_t y;

            union
            {
                struct
                {
                    bool direction;
                    int buttons;
                    bool doubleClick;
                } button;

                struct
                {
                    int32_t scrollX;
                    int32_t scrollY;
                } scroll;
            };
        } mouse;
    };
};

}

#endif //AWESOME_EVENT_H

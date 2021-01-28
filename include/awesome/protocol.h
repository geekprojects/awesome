#ifndef AWESOME_PROTOCOL_H
#define AWESOME_PROTOCOL_H

#include "event.h"

#define PACKED __attribute__((packed))

namespace Awesome
{

#define AWESOME_PROTOCOL_VERSION 1

enum MessageFlags
{
    MESSAGE_RESPONSE = 0x0,
    MESSAGE_REQUEST  = 0x1,
};

enum RequestType
{
    REQUEST_INFO              = 0x1000,
    REQUEST_INFO_DISPLAY      = 0x1001,

    REQUEST_SHM_ATTACH        = 0x2000,
    REQUEST_SHM_DETACH        = 0x2001,

    REQUEST_WINDOW_CREATE     = 0x3000,
    REQUEST_WINDOW_DESTROY    = 0x3001,
    REQUEST_WINDOW_UPDATE     = 0x3002,
    REQUEST_WINDOW_SET_SIZE   = 0x3003,
    REQUEST_WINDOW_SET_VISIBLE = 0x3004,

    REQUEST_EVENT_POLL        = 0x4000,
    REQUEST_EVENT_WAIT        = 0x4001

} PACKED;

struct Message
{
    uint16_t messageFlags = 0;
    uint16_t request = 0;
    uint32_t size = 0;
    uint64_t id = 0;

    Message() = default;
} PACKED;

struct Request : Message
{
    Request()
    {
        messageFlags = MESSAGE_REQUEST;
    }
} PACKED;;

struct Response : Message
{
    bool success = true;
    uint32_t error = 0;

    Response()
    {
        messageFlags = MESSAGE_RESPONSE;
    }

    Response(Request* srcRequest, bool _success, int _error = 0)
    {
        messageFlags = MESSAGE_RESPONSE;
        id = srcRequest->id;
        request = srcRequest->request;
        success = _success;
        error = _error;
    }
} PACKED;

#define MESSAGE_START(_type, _name, _requestType) \
struct _name##_type : _type \
{ \
    _name##_type() \
    { \
        request = _requestType; \
    }

#define MESSAGE_END } PACKED;

#define REQUEST_START(_name, _type) MESSAGE_START(Request, _name, _type)
#define REQUEST_END MESSAGE_END
#define REQUEST(_name, _type) REQUEST_START(_name, _type) REQUEST_END

#define RESPONSE_START(_name, _type) MESSAGE_START(Response, _name, _type)
#define RESPONSE_END MESSAGE_END
#define RESPONSE(_name, _type) RESPONSE_START(_name, _type) RESPONSE_END

REQUEST(Info, REQUEST_INFO)

RESPONSE_START(Info, REQUEST_INFO)
    uint32_t protocolVersion = AWESOME_PROTOCOL_VERSION;
    uint32_t numInterfaces = 0;
    uint32_t numDrivers = 0;
    uint32_t numDisplays = 0;

    char name[32] = {0};
    char vendor[32] = {0};
    char version[32] = {0};
RESPONSE_END

REQUEST_START(InfoDisplay, REQUEST_INFO_DISPLAY)
    uint32_t display = 0;
RESPONSE_END

RESPONSE_START(InfoDisplay, REQUEST_INFO_DISPLAY)
    uint32_t display = 0;
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    float scale = 0;
RESPONSE_END

REQUEST_START(ShmAttach, REQUEST_SHM_ATTACH)
    char path[256] = {0};
    int size = 0;
REQUEST_END

struct ShmDetachRequest : Request
{
    char path[256] = {0};

    ShmDetachRequest()
    {
        request = REQUEST_SHM_DETACH;
    }
} PACKED;

enum WindowFlags
{
    WINDOW_BORDER     = 0x0001,
    WINDOW_TITLE      = 0x0002,
    WINDOW_RESIZEABLE = 0x0004,
    WINDOW_POPUP      = 0x0008,
    WINDOW_BACKGROUND = 0x0010,
    WINDOW_FOREGROUND = 0x0020,
    WINDOW_FULLSCREEN = 0x1000,

    WINDOW_MOTION_EVENTS = 0x2000,

    WINDOW_MENU_BAR   = 0x8000,

    WINDOW_NORMAL     = WINDOW_BORDER | WINDOW_TITLE | WINDOW_RESIZEABLE
} PACKED;

#define WINDOW_POSITION_ANY INT32_MIN

struct WindowCreateRequest : Request
{
    int32_t x = WINDOW_POSITION_ANY;
    int32_t y = WINDOW_POSITION_ANY;
    int32_t width = 0;
    int32_t height = 0;
    uint64_t flags = WINDOW_NORMAL;
    uint8_t visible = true;
    wchar_t title[100] = {0};

    WindowCreateRequest()
    {
        request = REQUEST_WINDOW_CREATE;
    }
} PACKED;

struct WindowCreateResponse : Response
{
    uint64_t windowId;

    WindowCreateResponse()
    {
        request = REQUEST_WINDOW_CREATE;
    }
} PACKED;

struct WindowDestroyRequest : Request
{
    uint64_t windowId;

    WindowDestroyRequest()
    {
        request = REQUEST_WINDOW_DESTROY;
    }
} PACKED;

struct WindowDestroyResponse : Response
{
    uint64_t windowId;

    WindowDestroyResponse()
    {
        request = REQUEST_WINDOW_DESTROY;
    }
} PACKED;

struct WindowUpdateRequest : Request
{
    uint64_t windowId;
    char shmPath[256];
    int width;
    int height;

    WindowUpdateRequest()
    {
        request = REQUEST_WINDOW_UPDATE;
    }
} PACKED;

struct WindowSetSizeRequest : Request
{
    uint64_t windowId;
    int32_t width;
    int32_t height;

    WindowSetSizeRequest()
    {
        request = REQUEST_WINDOW_SET_SIZE;
    }
} PACKED;

struct WindowSetVisibleRequest : Request
{
    uint64_t windowId;
    uint8_t visible;

    WindowSetVisibleRequest()
    {
        request = REQUEST_WINDOW_SET_VISIBLE;
    }
} PACKED;

struct EventPollRequest : Request
{
    EventPollRequest()
    {
        request = REQUEST_EVENT_POLL;
    }
} PACKED;

struct EventWaitRequest : Request
{
    uint32_t timeout;

    EventWaitRequest()
    {
        request = REQUEST_EVENT_WAIT;
    }
} PACKED;

struct EventResponse : Response
{
    bool hasEvent;
    Event event;

    EventResponse()
    {
        request = REQUEST_EVENT_POLL;
    }
} PACKED;


}

#endif //AWESOME_PROTOCOL_H

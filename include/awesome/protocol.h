#ifndef AWESOME_PROTOCOL_H
#define AWESOME_PROTOCOL_H

#include <stdint.h>
#include "event.h"

#define PACKED __attribute__((packed))

namespace Awesome
{

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

    REQUEST_EVENT_POLL        = 0x4000,
    REQUEST_EVENT_WAIT        = 0x4001

} PACKED;

struct Message
{
    uint16_t messageFlags;
    uint16_t request;
    uint32_t size;
    uint64_t id;
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
    bool success;
    uint32_t error;

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

struct InfoRequest : Request
{
    InfoRequest()
    {
        request = REQUEST_INFO;
    }
} PACKED;

struct InfoResponse : Response
{
    uint32_t protocolVersion;
    uint32_t numInterfaces;
    uint32_t numDrivers;
    uint32_t numDisplays;

    char name[32];
    char vendor[32];
    char version[32];

    InfoResponse()
    {
        request = REQUEST_INFO;
    }
} PACKED;

struct InfoDisplayRequest : Request
{
    uint32_t display;

    InfoDisplayRequest()
    {
        request = REQUEST_INFO_DISPLAY;
    }
} PACKED;

struct InfoDisplayResponse : Response
{
    uint32_t display;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    float scale;

    InfoDisplayResponse()
    {
        request = REQUEST_INFO_DISPLAY;
    }
} PACKED;

struct ShmAttachRequest : Request
{
    char path[256];
    int size;

    ShmAttachRequest()
    {
        request = REQUEST_SHM_ATTACH;
    }
} PACKED;

struct ShmDetachRequest : Request
{
    char path[256];

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

    WINDOW_NORMAL     = WINDOW_BORDER | WINDOW_TITLE | WINDOW_RESIZEABLE
} PACKED;

struct WindowCreateRequest : Request
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint64_t flags = WINDOW_NORMAL;
    wchar_t title[100];

    WindowCreateRequest()
    {
        request = REQUEST_WINDOW_CREATE;
        title[0] = 0;
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

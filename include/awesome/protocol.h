#ifndef AWESOME_PROTOCOL_H
#define AWESOME_PROTOCOL_H

#include <stdint.h>
#include "event.h"

#define PACKED __attribute__((packed))

namespace Awesome
{

enum RequestType
{
    REQUEST_INFO              = 0x1000,

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
    bool direction;
    uint16_t request;
    uint64_t id;
    uint32_t size;
} __attribute__((packed));

struct Request : Message
{
    Request()
    {
        direction = true;
    }
};

struct Response : Message
{
    bool success;
    int error;

    Response()
    {
        direction = false;
    }

    Response(Request* srcRequest, bool _success, int _error = 0)
    {
        direction = false;
        id = srcRequest->id;
        request = srcRequest->request;
        success = _success;
        error = _error;
    }
};

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
};

struct ShmAttachRequest : Request
{
    char path[256];
    int size;

    ShmAttachRequest()
    {
        request = REQUEST_SHM_ATTACH;
    }
};

struct ShmDetachRequest : Request
{
    char path[256];

    ShmDetachRequest()
    {
        request = REQUEST_SHM_DETACH;
    }
};

struct WindowCreateRequest : Request
{
    int width;
    int height;
    int flags;

    WindowCreateRequest()
    {
        request = REQUEST_WINDOW_CREATE;
    }
};

struct WindowCreateResponse : Response
{
    int windowId;

    WindowCreateResponse()
    {
        request = REQUEST_WINDOW_CREATE;
    }
};

struct WindowUpdateRequest : Request
{
    int windowId;
    char shmPath[256];

    WindowUpdateRequest()
    {
        request = REQUEST_WINDOW_UPDATE;
    }
};

struct WindowSetSizeRequest : Request
{
    int windowId;
    int width;
    int height;

    WindowSetSizeRequest()
    {
        request = REQUEST_WINDOW_SET_SIZE;
    }
};

struct EventPollRequest : Request
{
    EventPollRequest()
    {
        request = REQUEST_EVENT_POLL;
    }
};

struct EventWaitRequest : Request
{
    uint32_t timeout;

    EventWaitRequest()
    {
        request = REQUEST_EVENT_WAIT;
    }
};

struct EventResponse : Response
{
    bool hasEvent;
    Event event;

    EventResponse()
    {
        request = REQUEST_EVENT_POLL;
    }
};


}

#endif //AWESOME_PROTOCOL_H

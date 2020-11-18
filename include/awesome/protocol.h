#ifndef AWESOME_PROTOCOL_H
#define AWESOME_PROTOCOL_H

#include <stdint.h>

#define PACKED __attribute__((packed))

namespace Awesome
{

enum RequestType
{
    REQUEST_INFO,

    REQUEST_SHM_ATTACH,
    REQUEST_SHM_DETACH,

    REQUEST_WINDOW_CREATE,
    REQUEST_WINDOW_DESTROY,
    REQUEST_WINDOW_UPDATE,
    REQUEST_WINDOW_SET_SIZE,

} PACKED;

struct Message
{
    bool direction;
    uint16_t request;
    uint64_t id;
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
    int shmid;
    int size;

    ShmAttachRequest()
    {
        request = REQUEST_SHM_ATTACH;
    }
};

struct ShmDetachRequest : Request
{
    int shmid;

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
    int shmid;

    WindowUpdateRequest()
    {
        request = REQUEST_WINDOW_UPDATE;
    }
};


}

#endif //AWESOME_PROTOCOL_H

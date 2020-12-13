
#include "awesome.h"
#include <awesome/displayserver.h>
#include <awesome/window.h>

#include <geek/gfx-surface.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

AwesomeClient::AwesomeClient(AwesomeInterface* awesomeInterface, int fd, bufferevent* bev) : Awesome::Client(awesomeInterface), Logger("AwesomeClient")
{
    m_fd = fd;
    m_bufferEvent = bev;
}

AwesomeClient::~AwesomeClient()
{
}

void AwesomeClient::readCallback(struct bufferevent* bev, void* ctx)
{
    ((AwesomeClient*)ctx)->readCallback(bev);
}

static int g_total = 0;
void AwesomeClient::readCallback(bufferevent* bev)
{
    evbuffer* input = bufferevent_get_input(bev);

    size_t len = evbuffer_get_length(input);
    g_total += len;

    if (len < sizeof(Request))
    {
        log(ERROR, "readCallback: Message is too short!");
        evbuffer_drain(input, len);
        return;
    }

    void* buffer = malloc(len);
    evbuffer_remove(input, buffer, len);
    Request* request = (Request*)buffer;

    //log(DEBUG, "readCallback: request type: 0x%x, id: %d", request->request, request->id);
    if (!(request->messageFlags & MESSAGE_REQUEST))
    {
        log(ERROR, "readCallback: Received response message?");
        free(buffer);
        return;
    }

    switch(request->request)
    {
        case REQUEST_INFO:
            handleInfoRequest((InfoRequest*)request);
            break;

        case REQUEST_INFO_DISPLAY:
            handleInfoDisplayRequest((InfoDisplayRequest*)request);
            break;

        case REQUEST_SHM_ATTACH:
            handleSharedMemoryAttach((ShmAttachRequest*)request);
            break;

        case REQUEST_SHM_DETACH:
            handleSharedMemoryDetach((ShmDetachRequest*)request);
            break;

        case REQUEST_WINDOW_CREATE:
            handleWindowCreate((WindowCreateRequest*)request);
            break;

        case REQUEST_WINDOW_UPDATE:
            handleWindowUpdate((WindowUpdateRequest*)request);
            break;

        case REQUEST_WINDOW_SET_SIZE:
            handleWindowSetSize((WindowSetSizeRequest*)request);
            break;

        case REQUEST_EVENT_POLL:
            handleEventPoll((EventPollRequest*)request);
            break;

        case REQUEST_EVENT_WAIT:
            handleEventWait((EventWaitRequest*)request);
            break;

        default:
            log(DEBUG, "readCallback: Unhandled request: %d", request->request);
    }

    free(buffer);
}

void AwesomeClient::errorCallback(bufferevent *bev, short error, void *ctx)
{
    ((AwesomeClient*)ctx)->errorCallback(bev, error);
}

void AwesomeClient::errorCallback(bufferevent* bev, short error)
{
    log(DEBUG, "errorCallback: error=0x%x", error);

    if (error & BEV_EVENT_EOF)
    {
        /* connection has been closed, do any clean up here */
        log(DEBUG, "errorCallback: EOF");
        m_interface->getDisplayServer()->removeClient(this);
    }
    else if (error & BEV_EVENT_ERROR)
    {
        /* check errno to see what error occurred */
        log(ERROR, "errorCallback: Event error!");
    }
    else if (error & BEV_EVENT_TIMEOUT)
    {
        /* must be a timeout event handle, handle it */
        log(WARN, "errorCallback: Timeout!");
    }
    else
    {
        log(WARN, "errorCallback: Unknown error: 0x%x", error);
    }

    bufferevent_free(bev);
}

void AwesomeClient::handleInfoRequest(InfoRequest* request)
{
    log(DEBUG, "handleInfoRequest: REQUEST_INFO: Sending info!");

    InfoResponse response;
    response.request = REQUEST_INFO;
    response.id = request->id;
    response.protocolVersion = 1;

    vector<Display*> displays = m_interface->getDisplayServer()->getDisplays();
    response.numDisplays = displays.size();

    response.numDrivers = 1;
    response.numInterfaces = 1;
    strncpy(response.name, "Awesome", 32);
    strncpy(response.version, "0.1", 32);
    strncpy(response.vendor, "GeekProjects", 32);

    send(&response, sizeof(response));
}

void AwesomeClient::handleInfoDisplayRequest(InfoDisplayRequest* request)
{
    vector<Display*> displays = m_interface->getDisplayServer()->getDisplays();
    log(DEBUG, "handleInfoDisplayRequest: display=%d, displays=%d", request->display, displays.size());
    if (request->display >= displays.size())
    {
        Response response(request, false);
        send(&response, sizeof(response));
        return;
    }

    Display* display = displays.at(request->display);

    InfoDisplayResponse response;
    response.request = REQUEST_INFO_DISPLAY;
    response.success = true;
    response.id = request->id;
    response.x = display->getRect().x;
    response.y = display->getRect().y;
    response.width = display->getRect().w;
    response.height = display->getRect().h;
    send(&response, sizeof(response));
}

void AwesomeClient::handleSharedMemoryAttach(ShmAttachRequest* request)
{
    log(DEBUG, "handleSharedMemoryAttach: path=%s", request->path);

    int fd = shm_open(request->path, O_RDONLY, 0666);
    if (fd == -1)
    {
        log(DEBUG, "handleSharedMemoryAttach: Failed to open shared memory: %s", strerror(errno));
        Response response(request, false);
        send(&response, sizeof(response));
        return;
    }

    SharedMemory sharedMemory;
    sharedMemory.addr = mmap(nullptr, request->size, PROT_READ , MAP_SHARED, fd, 0);
    sharedMemory.size = request->size;
    log(DEBUG, "handleSharedMemoryAttach: addr=%p", sharedMemory.addr);

    m_sharedMemory.insert(make_pair(request->path, sharedMemory));

    Response response(request, true);
    send(&response, sizeof(response));
}

void AwesomeClient::handleSharedMemoryDetach(ShmDetachRequest* request)
{
    auto it = m_sharedMemory.find(request->path);
    if (it == m_sharedMemory.end())
    {
        log(WARN, "handleSharedMemoryDetach: Unknown shared memory id: %d", request->path);
        return;
    }

    //shmdt(it->second);
    munmap(it->second.addr, it->second.size);

    m_sharedMemory.erase(it);

    Response response(request, true);
    send(&response, sizeof(response));
}

void AwesomeClient::handleWindowCreate(WindowCreateRequest* request)
{
    Window* window = new Window(m_interface->getDisplayServer(), this);
    window->setPosition(Vector2D(0, 0));
    window->setFlags(request->flags);
    window->setTitle(request->title);
    window->setContentSize(request->width, request->height);

    m_interface->getDisplayServer()->getCompositor()->addWindow(window);

    log(DEBUG, "handleWindowCreate: windowId=%d, width=%d, height=%d, title: %ls", window->getId(), request->width, request->height, window->getTitle().c_str());

    WindowCreateResponse response;
    response.id = request->id;
    response.success = true;
    response.windowId = window->getId();

    send(&response, sizeof(response));
}

void AwesomeClient::handleWindowUpdate(WindowUpdateRequest* request)
{
    log(DEBUG, "handleWindowUpdate: windowId=%d, shmPath=%s", request->windowId, request->shmPath);
    Window* window = getWindow(request->windowId);
    if (window == nullptr)
    {
        log(ERROR, "handleWindowUpdate: Invalid windowId: %d", request->windowId);
        Response response(request, false);
        send(&response, sizeof(response));
        return;
    }

    auto it = m_sharedMemory.find(request->shmPath);
    if (it == m_sharedMemory.end())
    {
        log(ERROR, "handleWindowUpdate: Invalid shmid: %s", request->shmPath);
        Response response(request, false);
        send(&response, sizeof(response));
        return;
    }

    float scale = 2.0f;
    Surface* surface = new Surface(
        window->getContentRect().w * scale,
        window->getContentRect().h * scale,
        4,
        (uint8_t*)it->second.addr);

    window->update(surface);

    m_interface->getDisplayServer()->getDrawSignal()->signal();

    delete surface;

    Response response(request, true);
    send(&response, sizeof(response));
}

Window* AwesomeClient::getWindow(int windowId) const
{
    Window* window = m_interface->getDisplayServer()->getCompositor()->findWindow(windowId);
    return window;
}

void AwesomeClient::handleWindowSetSize(WindowSetSizeRequest* request)
{
    log(DEBUG, "handleWindowSetSize: windowId=%d, size=%d, %d", request->windowId, request->width, request->height);
    Window* window = getWindow(request->windowId);
    if (window == nullptr)
    {
        log(ERROR, "handleWindowSetSize: Invalid windowId: %d", request->windowId);
        Response response(request, false);
        send(&response, sizeof(response));
        return;
    }

    window->setContentSize(request->width, request->height);

    Response response(request, true);
    send(&response, sizeof(response));
}

void AwesomeClient::handleEventPoll(EventPollRequest* request)
{
    EventResponse response;
    response.request = REQUEST_EVENT_POLL;
    response.id = request->id;
    response.success = true;

    Event* event = popEvent();
    if (event == nullptr)
    {
        response.hasEvent = false;
        send(&response, sizeof(response));
        return;
    }

    response.hasEvent = true;
    response.event = *event;

    log(DEBUG, "handleEventPoll: Found event!");
    send(&response, sizeof(response));
}

void AwesomeClient::handleEventWait(EventWaitRequest* request)
{
    Event* event = popEvent();
    if (event != nullptr)
    {
        EventResponse response;
        response.request = REQUEST_EVENT_WAIT;
        response.id = request->id;
        response.success = true;
        response.hasEvent = true;
        response.event = *event;
        send(&response, sizeof(response));
        m_waitingForEvent = false;
        return;
    }

    log(DEBUG, "handleEventWait: %p: No current event, waiting...", this);
    m_waitingForEvent = true;
    m_waitingForEventRequestId = request->id;
}

void AwesomeClient::send(Message* message, int size)
{
    log(DEBUG, "send: %p: Sending %d bytes", this, size);
    message->size = size;

    bufferevent_write(m_bufferEvent, message, size);
    bufferevent_flush(m_bufferEvent, EV_WRITE, BEV_FLUSH);

    /*
    //int res = ::send(m_fd, message, size, 0);
    int res = ::write(m_fd, message, size);
    log(DEBUG, "send: res=%d", res);
     */
}

bool AwesomeClient::receivedEvent(Event* event)
{
    log(DEBUG, "receivedEvent: %p: waiting=%d", this, m_waitingForEvent);
    if (m_waitingForEvent)
    {
        EventResponse response;
        response.request = REQUEST_EVENT_WAIT;
        response.id = m_waitingForEventRequestId;
        response.success = true;
        response.hasEvent = true;
        response.event = *event;

        log(DEBUG, "receivedEvent: Sending...");
        send(&response, sizeof(response));
        m_waitingForEvent = false;
        return false;
    }

    return true;
}



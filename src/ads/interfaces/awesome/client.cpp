
#include "awesome.h"
#include <awesome/displayserver.h>
#include <awesome/window.h>

#include <geek/gfx-surface.h>

#include <sys/shm.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

AwesomeClient::AwesomeClient(AwesomeInterface* awesomeInterface, bufferevent* bev) : Awesome::Client(awesomeInterface), Logger("AwesomeClient")
{
    m_bufferEvent = bev;
}

AwesomeClient::~AwesomeClient()
{
}

void AwesomeClient::readCallback(struct bufferevent* bev, void* ctx)
{
    ((AwesomeClient*)ctx)->readCallback(bev);
}

void AwesomeClient::readCallback(bufferevent* bev)
{
    log(DEBUG, "readCallback: bev=%p", bev);
    evbuffer* input = bufferevent_get_input(bev);

    size_t len = evbuffer_get_length(input);
    log(DEBUG, "readCallback: Got %d bytes!", len);

    if (len < sizeof(Request))
    {
        log(ERROR, "readCallback: Message is too short!");
        evbuffer_drain(input, len);
        return;
    }

    void* buffer = malloc(len);
    evbuffer_remove(input, buffer, len);
    Request* request = (Request*)buffer;

    if (!request->direction)
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

        default:
            log(DEBUG, "readCallback: Unhandled request: %d", request->request);
    }

    log(DEBUG, "readCallback: request type: %d, id: %d", request->request, request->id);
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
        log(DEBUG, "errorCallback: EOF");
        m_interface->getDisplayServer()->removeClient(this);
        //m_interface->get->addClient(client);
        /* connection has been closed, do any clean up here */
        /* ... */
    }
    else if (error & BEV_EVENT_ERROR)
    {
        log(ERROR, "errorCallback: Event error!");
        /* check errno to see what error occurred */
        /* ... */
    }
    else if (error & BEV_EVENT_TIMEOUT)
    {
        log(WARN, "errorCallback: Timeout!");
        /* must be a timeout event handle, handle it */
        /* ... */
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
    response.numDisplays = 1;
    response.numDrivers = 1;
    response.numInterfaces = 1;
    strncpy(response.name, "Awesome", 32);
    strncpy(response.version, "0.1", 32);
    strncpy(response.vendor, "GeekProjects", 32);

    evbuffer* output = bufferevent_get_output(m_bufferEvent);
    evbuffer_add(output, &response, sizeof(response));
}

void AwesomeClient::handleSharedMemoryAttach(ShmAttachRequest* request)
{
    log(DEBUG, "handleSharedMemoryAttach: shmid=%d", request->shmid);

    void* addr = shmat(request->shmid, 0, 0);
    log(DEBUG, "handleSharedMemoryAttach: addr=%p", addr);

    m_sharedMemory.insert(make_pair(request->shmid, addr));

    Response response(request, true);

    evbuffer* output = bufferevent_get_output(m_bufferEvent);
    evbuffer_add(output, &response, sizeof(response));
}

void AwesomeClient::handleSharedMemoryDetach(ShmDetachRequest* request)
{
    auto it = m_sharedMemory.find(request->shmid);
    if (it == m_sharedMemory.end())
    {
        log(WARN, "handleSharedMemoryDetach: Unknown shared memory id: %d", request->shmid);
        return;
    }

    shmdt(it->second);

    m_sharedMemory.erase(it);
}

void AwesomeClient::handleWindowCreate(WindowCreateRequest* request)
{
    Window* window = new Window(this);
    window->setRect(Rect(0, 0, request->width, request->height));

    m_interface->getDisplayServer()->getCompositor()->addWindow(window);

    log(DEBUG, "handleWindowCreate: windowId=%d, width=%d, height=%d", request->width, request->height);

    WindowCreateResponse response;
    response.id = request->id;
    response.success = true;
    response.windowId = window->getId();

    evbuffer* output = bufferevent_get_output(m_bufferEvent);
    evbuffer_add(output, &response, sizeof(response));
}

void AwesomeClient::handleWindowUpdate(WindowUpdateRequest* request)
{
    log(DEBUG, "handleWindowUpdate: windowId=%d, shmid=%d", request->windowId, request->shmid);
    Window* window = m_interface->getDisplayServer()->getCompositor()->findWindow(request->windowId);
    if (window == nullptr)
    {
        log(ERROR, "handleWindowUpdate: Invalid windowId: %d", request->windowId);
        return;
    }

    auto it = m_sharedMemory.find(request->shmid);
    if (it == m_sharedMemory.end())
    {
        log(ERROR, "handleWindowUpdate: Invalid shmid: %d", request->shmid);
        return;
    }

    float scale = 1.0f;
    Surface* surface = new Surface(window->getRect().w * scale, window->getRect().h * scale, 4, (uint8_t*)it->second);

    log(DEBUG, "handleWindowUpdate: Updating window: %p", window);
    window->update(surface);
}



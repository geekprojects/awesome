#include <awesome/connection.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

Connection::Connection() : Logger("Connection")
{
    m_requestMutex = Thread::createMutex();
}

Connection::~Connection()
{
    if (m_fd != -1)
    {
        close(m_fd);
    }
}

bool Connection::init()
{
    m_connectionThread = new ConnectionSendThread(this);
    m_connectionThread->start();

    m_connectionReceiveThread = new ConnectionReceiveThread(this);
    m_connectionReceiveThread->start();

    return true;
}

InfoResponse* Connection::getInfo()
{
    if (m_fd == -1)
    {
        log(ERROR, "getInfo: No connection");
        return nullptr;
    }

    InfoRequest requestInfo;
    requestInfo.request = REQUEST_INFO;
    requestInfo.direction = true;
    requestInfo.id = 1;

    Response* message = send(&requestInfo, sizeof(requestInfo));
    if (message != nullptr)
    {
        log(DEBUG, "getInfo: packet=%p", message);

        InfoResponse* response = new InfoResponse();
        memcpy(response, message, sizeof(InfoResponse));

        free(message);

        return response;
    }

    return nullptr;
}

Response* Connection::send(Request* message, int size)
{
    log(DEBUG, "send: Sending message...");
    ConnectionRequest* connectionRequest = m_connectionThread->send(message, size);

    log(DEBUG, "send: Waiting for response to %d", connectionRequest->request->id);
    if (connectionRequest->response == nullptr)
    {
        connectionRequest->signal->wait();
    }

    log(DEBUG, "send: Got response!");
    Response* response = connectionRequest->response;
    delete connectionRequest;

    return response;
}

void Connection::receivedResponse(Response* response)
{
    log(DEBUG, "receivedResponse: Got response: id=%d", response->id);
    m_requestMutex->lock();

    auto it = m_requests.find(response->id);
    if (it == m_requests.end())
    {
        log(WARN, "receivedResponse: Unknown request id: %d", response->id);
        return;
    }

    ConnectionRequest* connectionRequest = it->second;
    m_requests.erase(it);
    m_requestMutex->unlock();

    connectionRequest->response = response;
    log(DEBUG, "receivedResponse: Done with request: %d, signalling", response->id);
    connectionRequest->signal->signal();
}

void Connection::addRequest(ConnectionRequest* connectionRequest)
{
    m_requestMutex->lock();
    m_requests.insert(make_pair(connectionRequest->request->id, connectionRequest));
    m_requestMutex->unlock();
}

ClientConnection::ClientConnection(string connectionStr)
{
    m_connectionStr = connectionStr;
}

ClientConnection::~ClientConnection()
{
}

bool ClientConnection::connect()
{
    bool res;
    if (m_connectionStr.substr(0, 5) == "unix:")
    {
        log(DEBUG, "connect: Unix socket!");
        res = connectUnix();
        if (!res)
        {
            return false;
        }
    }
    else
    {
        log(ERROR, "connect: Unhandled connection string: %s", m_connectionStr.c_str());
        return false;
    }

    init();

    return true;
}

bool ClientConnection::connectUnix()
{
    string path = m_connectionStr.substr(5);
    log(DEBUG, "connectUnix: path=%s", path.c_str());

    m_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    log(DEBUG, "connectUnix: m_fd=%d", m_fd);

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)-1);

    int res;
    res = ::connect(m_fd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    int err = errno;
    log(DEBUG, "connectUnix: res=%d, errno=%d", res, err);

    return true;
}

bool ClientConnection::connectINet()
{
    return false;
}

static void randname(char *buf)
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i)
    {
        buf[i] = 'A' + (r & 15) + (r & 16) * 2;
        r >>= 5;
    }
}

ClientSharedMemory* ClientConnection::createSharedMemory(int size)
{
    ShmAttachRequest request;

    int fd = -1;
    char name[] = "/awesome_shm-XXXXXX";
    int retries = 100;

    do {
        randname(name + sizeof(name) - 7);
        --retries;
        fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0)
        {
            //shm_unlink(name);
            break;
        }
    }
    while (retries > 0 && errno == EEXIST);

    if (fd == -1)
    {
        return nullptr;
    }

    int res;
    res = ftruncate(fd, size);
    if (res != 0)
    {
        log(ERROR, "Failed to set size: %s", strerror(errno));
        return nullptr;
    }

    strncpy(request.path, name, 255);
    request.size = size;

    Response* response = send(&request, sizeof(request));
    log(DEBUG, "createSharedMemory: success=%d", response->success);

    void* shmaddr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    return new ClientSharedMemory(this, name, fd, size, shmaddr);
}

void ClientConnection::destroySharedMemory(ClientSharedMemory* csm)
{
    ShmDetachRequest shmDetachRequest;
    strncpy(shmDetachRequest.path, csm->getPath().c_str(), 255);

    send(&shmDetachRequest, sizeof(shmDetachRequest));

    munmap(csm->getAddr(), csm->getSize());

    shm_unlink(csm->getPath().c_str());
}

Event* ClientConnection::eventPoll()
{
    log(DEBUG, "eventPoll: Polling..");
    EventPollRequest request;

    EventResponse* eventPollResponse = (EventResponse*)send(&request, sizeof(request));

    Event* event = nullptr;
    if (eventPollResponse->hasEvent)
    {
        event = new Event();
        memcpy((Event*)event, &(eventPollResponse->event), sizeof(Event));
        log(DEBUG, "eventPoll: Received event!");
    }

    free(eventPollResponse);

    return event;
}

Event* ClientConnection::eventWait()
{
    log(DEBUG, "eventWait: Sending wait request..");
    EventWaitRequest request;
    EventResponse* eventPollResponse = (EventResponse*)send(&request, sizeof(request));

    log(DEBUG, "eventWait: Received message");

    Event* event = nullptr;
    if (eventPollResponse->hasEvent)
    {
        event = new Event();
        memcpy((Event*)event, &(eventPollResponse->event), sizeof(Event));
        log(DEBUG, "eventPoll: Received event!");
    }
    else
    {
        log(DEBUG, "eventWait: No event?");
    }

    free(eventPollResponse);

    return event;
}

ClientSharedMemory::ClientSharedMemory(
    ClientConnection* connection,
    string path,
    int fd,
    int size,
    void* addr)
{
    m_connection = connection;
    m_path = path;
    m_fd = fd;
    m_size = size;
    m_addr = addr;
}

ClientSharedMemory::~ClientSharedMemory()
{
}

#include <awesome/connection.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

Connection::Connection() : Logger("Connection")
{
    m_requestMutex = Thread::createMutex();
}

Connection::~Connection()
{
    close();
}

bool Connection::init()
{
    m_connectionSendThread = new ConnectionSendThread(this);
    m_connectionSendThread->start();

    m_connectionReceiveThread = new ConnectionReceiveThread(this);
    m_connectionReceiveThread->start();

    return true;
}

void Connection::close()
{
    if (m_fd != -1)
    {
        ::close(m_fd);
    }

    m_fd = -1;

    m_connectionReceiveThread->stop();
    m_connectionSendThread->stop();

    m_requestMutex->lock();

    for (auto it : m_requests)
    {
        it.second->signal->signal();
    }
    m_requests.clear();

    m_requestMutex->unlock();
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
    requestInfo.id = 1;

    Response* message = send(&requestInfo, sizeof(requestInfo));
    if (message != nullptr)
    {
        auto response = new InfoResponse();
        memcpy(response, message, sizeof(InfoResponse));

        free(message);

        return response;
    }

    return nullptr;
}

Response* Connection::send(Request* message, int size)
{
    volatile ConnectionRequest* connectionRequest = m_connectionSendThread->send(message, size);

    m_requestMutex->lock();
    Response* response = connectionRequest->response;
    m_requestMutex->unlock();
    if (response == nullptr)
    {
        connectionRequest->signal->wait();
    }
    else
    {
        log(DEBUG, "send: Already got a response!");
    }

    response = connectionRequest->response;
    delete connectionRequest;

    return response;
}

void Connection::receivedResponse(Response* response)
{
    m_requestMutex->lock();

    auto it = m_requests.find(response->id);
    if (it == m_requests.end())
    {
        m_requestMutex->unlock();
        log(WARN, "receivedResponse: Unknown request id: %d", response->id);
        return;
    }

    ConnectionRequest* connectionRequest = it->second;
    m_requests.erase(it);
    connectionRequest->response = response;
    m_requestMutex->unlock();

    connectionRequest->signal->signal();

}

void Connection::addRequest(ConnectionRequest* connectionRequest)
{
    m_requestMutex->lock();
    int id = connectionRequest->request->id;
    m_requests.insert(make_pair(id, connectionRequest));
    m_requestMutex->unlock();
}

ClientConnection::ClientConnection(const string& connectionStr)
{
    m_connectionStr = connectionStr;
}

ClientConnection::~ClientConnection() = default;

bool ClientConnection::connect()
{
    bool res;

    int pos = m_connectionStr.find(':');
    if (pos == string::npos)
    {
        return false;
    }

    string type = m_connectionStr.substr(0, pos);
    if (type == "unix")
    {
        res = connectUnix();
        if (!res)
        {
            return false;
        }
    }
    else if (type == "inet")
    {
        res = connectINet();
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
    string path = m_connectionStr.substr(5);
    log(DEBUG, "connectInet: path=%s", path.c_str());

    int port = 16523;
    string host = path;

    int pos = path.find(':');
    log(DEBUG, "connectINet: pos=%d", pos);
    if (pos != string::npos)
    {
        host = path.substr(0, pos);
        string portStr = path.substr(pos + 1);
        port = atoi(portStr.c_str());
    }
    log(DEBUG, "connectINet: host=%s, port=%d", host.c_str(), port);

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0)
    {
        log(ERROR, "connectINet: Failed to create socket: %s", strerror(errno));
        return false;
    }

    sockaddr_in sa;
    memset(&sa, 0, sizeof(struct sockaddr_in));
    hostent* he = gethostbyname(host.c_str());
    sa.sin_addr = *((struct in_addr *)he->h_addr);
    sa.sin_port = htons(port);

    int res;
    res = ::connect(m_fd, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa));
    if (res == -1)
    {
        int err = errno;
        log(DEBUG, "connectINet: res=%d, errno=%d", res, err);
        return false;
    }

    log(DEBUG, "connectINet: connected! (%d)", m_fd);

    return true;
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
    if (!isOpen())
    {
        return nullptr;
    }

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

    return new ClientSharedMemory(name, fd, size, shmaddr);
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
    if (!isOpen())
    {
        return nullptr;
    }

    EventPollRequest request;

    auto eventPollResponse = (EventResponse*)send(&request, sizeof(request));

    Event* event = nullptr;
    if (eventPollResponse->hasEvent)
    {
        event = new Event();
        memcpy((Event*)event, &(eventPollResponse->event), sizeof(Event));
    }

    free(eventPollResponse);

    return event;
}

Event* ClientConnection::eventWait()
{
    if (!isOpen())
    {
        return nullptr;
    }

    EventWaitRequest request;
    auto eventPollResponse = (EventResponse*)send(&request, sizeof(request));
    if (eventPollResponse == nullptr)
    {
        return nullptr;
    }

    Event* event = nullptr;
    if (eventPollResponse->hasEvent)
    {
        event = new Event();
        memcpy((Event*)event, &(eventPollResponse->event), sizeof(Event));
    }
    else
    {
        log(DEBUG, "eventWait: No event?");
    }

    free(eventPollResponse);

    return event;
}

ClientSharedMemory::ClientSharedMemory(
    string path,
    int fd,
    int size,
    void* addr)
{
    m_path = path;
    m_fd = fd;
    m_size = size;
    m_addr = addr;
}

ClientSharedMemory::~ClientSharedMemory() = default;

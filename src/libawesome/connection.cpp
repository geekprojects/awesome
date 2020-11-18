//
// Created by Ian Parker on 16/11/2020.
//

#include <awesome/connection.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <unistd.h>
#include <errno.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

Connection::Connection() : Logger("Connection")
{
}

Connection::~Connection()
{
    if (m_fd != -1)
    {
        close(m_fd);
    }
}

InfoResponse* Connection::getInfo()
{
    if (m_fd == -1)
    {
        log(ERROR, "getInfo: No connection");
        return NULL;
    }

    InfoRequest requestInfo;
    requestInfo.request = REQUEST_INFO;
    requestInfo.direction = true;
    requestInfo.id = 1;

    send(&requestInfo, sizeof(requestInfo));

    Message* message = wait();
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

bool Connection::send(Message* message, int size)
{
    message->id = m_messageId++;
    int res = ::send(m_fd, message, size, 0);
    return (res >= 0);
}

Message* Connection::wait()
{
    fd_set fd_set;
    FD_ZERO (&fd_set);
    FD_SET (m_fd, &fd_set);

    int res;
    res = select(FD_SETSIZE, &fd_set, NULL, NULL, NULL);
    if (res < 0)
    {
        log(ERROR, "wait: select failed!");
        return nullptr;
    }

    if (FD_ISSET (m_fd, &fd_set))
    {
        int readySize = 0;
        ioctl(m_fd, FIONREAD, &readySize);
        log(DEBUG, "wait: readySize=%d", readySize);

        if (readySize > 0)
        {
            void* buffer = malloc(readySize);
            read(m_fd, buffer, readySize);

            return (Response*)buffer;
        }
    }

    return nullptr;
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
    if (m_connectionStr.substr(0, 5) == "unix:")
    {
        log(DEBUG, "connect: Unix socket!");
        return connectUnix();
    }
    log(ERROR, "connect: Unhandled connection string: %s", m_connectionStr.c_str());
    return false;
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

    return false;
}

bool ClientConnection::connectINet()
{
    return false;
}

ClientSharedMemory* ClientConnection::createSharedMemory(int size)
{
    ShmAttachRequest request;
    request.shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (request.shmid == -1)
    {
        log(ERROR, "createSharedMemory: Failed to create shared memory: %s", strerror(errno));
        return nullptr;
    }
    request.size = size;

    send(&request, sizeof(request));
    Response* response = static_cast<Response*>(wait());

    log(DEBUG, "createSharedMemory: success=%d", response->success);

    void* shmaddr = shmat(request.shmid, 0, 0);
    return new ClientSharedMemory(this, request.shmid, size, shmaddr);
}

void ClientConnection::destroySharedMemory(ClientSharedMemory* csm)
{
    ShmDetachRequest shmDetachRequest;
    shmDetachRequest.shmid = csm->getShmid();

    send(&shmDetachRequest, sizeof(shmDetachRequest));

    shmdt(csm->getAddr());
    shmctl(csm->getShmid(), IPC_RMID, 0);
}

ClientSharedMemory::ClientSharedMemory(ClientConnection* connection, int shmid, int size, void* addr)
{
    m_connection = connection;
    m_shmid = shmid;
    m_size = size;
    m_addr = addr;
}

ClientSharedMemory::~ClientSharedMemory()
{
}

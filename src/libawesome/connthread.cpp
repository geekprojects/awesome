//
// Created by Ian Parker on 24/11/2020.
//

#include <awesome/connection.h>

#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

ConnectionSendThread::ConnectionSendThread(Connection* connection) : Logger("ConnectionSendThread")
{
    m_connection = connection;

    m_requestMutex = Thread::createMutex();
    m_requestSignal = Thread::createCondVar();
}

ConnectionSendThread::~ConnectionSendThread() noexcept
{
}

bool ConnectionSendThread::main()
{
    log(DEBUG, "main: Starting!");
    m_running = true;
    while (m_running)
    {
        bool more = true;
        while (more)
        {
            m_requestMutex->lock();
            if (!m_requestQueue.empty())
            {
                ConnectionRequest* request = m_requestQueue.front();
                m_requestQueue.pop_front();

                m_requestMutex->unlock();

                log(DEBUG, "main: Sending request %d", request->request->id);
                int res = ::send(m_connection->getFD(), request->request, request->requestSize, 0);
                if (res < 0)
                {
                    log(ERROR, "main: Failed to send: %s", strerror(errno));
                }

            }
            else
            {
                m_requestMutex->unlock();
                more = false;
            }
        }

        log(DEBUG, "main: Waiting for something to send");
        m_requestSignal->wait();
        log(DEBUG, "main: Woken up!");
    }

    return true;
}

ConnectionRequest* ConnectionSendThread::send(Request* request, int size)
{
    log(DEBUG, "send: Got request!");
    m_requestMutex->lock();
    request->id = m_messageId++;
    request->size = size;

    ConnectionRequest* connectionRequest = new ConnectionRequest();
    connectionRequest->request = request;
    connectionRequest->requestSize = size;
    connectionRequest->signal = Thread::createCondVar();
    connectionRequest->response = nullptr;
    m_requestQueue.push_back(connectionRequest);
    m_requestMutex->unlock();

    // Make sure the Connection has the request before we send it!
    m_connection->addRequest(connectionRequest);

    m_requestSignal->signal();

    log(DEBUG, "send: Done!");

    return connectionRequest;
}

ConnectionReceiveThread::ConnectionReceiveThread(Connection* connection) : Geek::Logger("ConnectionReceiveThread")
{
    m_connection = connection;
}

ConnectionReceiveThread::~ConnectionReceiveThread()
{

}

bool ConnectionReceiveThread::main()
{
    log(DEBUG, "main: Starting up...");
    int fd = m_connection->getFD();

    fd_set fdSet;
    FD_ZERO (&fdSet);
    FD_SET (fd, &fdSet);

    m_running = true;
    while (m_running)
    {
        fd_set activeSet = fdSet;
        log(DEBUG, "main: Waiting for data...");
        int res;
        res = select(FD_SETSIZE, &activeSet, NULL, NULL, NULL);
        if (res < 0)
        {
            log(ERROR, "main: select failed!");
            return false;
        }

        if (FD_ISSET (fd, &activeSet))
        {
            int readySize = 0;
            ioctl(fd, FIONREAD, &readySize);
            log(DEBUG, "main: Got data! readySize=%d", readySize);

            if (readySize > 0)
            {
                char* buffer = (char*)malloc(readySize);
                read(fd, buffer, readySize);

                int remaining = readySize;
                char* ptr = buffer;
                while (remaining > 0)
                {
                    Message* msgPtr = (Message*)ptr;
                    void* messageBuffer = malloc(msgPtr->size);

                    memcpy(messageBuffer, msgPtr, msgPtr->size);
                    remaining -= msgPtr->size;

                    m_connection->receivedResponse((Response*)messageBuffer);
                }

                free(buffer);
            }
            else
            {
                break;
            }
        }
    }

    return true;
}

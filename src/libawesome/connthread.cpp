#include <awesome/connection.h>

#include <unistd.h>
#include <cerrno>
#include <cstring>
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

ConnectionSendThread::~ConnectionSendThread() = default;

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
                log(DEBUG, "main: Got message to send");
                ConnectionRequest* request = m_requestQueue.front();
                m_requestQueue.pop_front();

                m_requestMutex->unlock();

                int res = ::send(m_connection->getFD(), request->request, request->requestSize, 0);
                if (res < 0)
                {
                    log(ERROR, "main: Failed to send: %s", strerror(errno));
                    break;
                }
            }
            else
            {
                m_requestMutex->unlock();
                more = false;
            }
        }

        log(DEBUG, "main: Waiting for message");
        m_requestSignal->wait();
        log(DEBUG, "main: Woken up!");
    }

    m_connection->closed();

    return true;
}

ConnectionRequest* ConnectionSendThread::send(Request* request, int size)
{
    log(DEBUG, "send: Queuing...");
    m_requestMutex->lock();
    request->id = m_messageId++;
    request->size = size;

    ConnectionRequest* connectionRequest = new ConnectionRequest();
    connectionRequest->request = request;
    connectionRequest->requestSize = size;
    connectionRequest->signal = Thread::createCondVar();
    connectionRequest->response = nullptr;
    m_requestQueue.push_back(connectionRequest);

    // Make sure the Connection has the request before we send it!
    m_connection->addRequest(connectionRequest);

    m_requestMutex->unlock();

    log(DEBUG, "send: Signalling...");
    m_requestSignal->signal();

    log(DEBUG, "send: Done!");
    return connectionRequest;
}

ConnectionReceiveThread::ConnectionReceiveThread(Connection* connection) : Geek::Logger("ConnectionReceiveThread")
{
    m_connection = connection;
}

ConnectionReceiveThread::~ConnectionReceiveThread() = default;

bool ConnectionReceiveThread::main()
{
    int fd = m_connection->getFD();
    log(DEBUG, "main: Starting up... (fd=%d)", fd);

    fd_set fdSet;
    FD_ZERO (&fdSet);
    FD_SET (fd, &fdSet);

    m_running = true;
    while (m_running)
    {
#if 0
        int res;

        int readySize = 0;
        res = ioctl(fd, FIONREAD, &readySize);
        if (res < 0)
        {
            log(ERROR, "FIONREAD failed: %s", strerror(errno));
            break;
        }
        else if (readySize == 0)
        {
            log(DEBUG, "main: Waiting on select");
            fd_set activeSet = fdSet;
            res = select(FD_SETSIZE, &activeSet, nullptr, nullptr, nullptr);
            if (res < 0)
            {
                log(ERROR, "main: select failed: %s", strerror(errno));
                break;
            }
            /*
            if (FD_ISSET (fd, &activeSet))
            {
                continue;
            }
             */
        }
        else
        {
            log(INFO, "main: readySize=%d", readySize);
            char* buffer = (char*)malloc(readySize);
                res = read(fd, buffer, readySize);
                if (res < 0)
                {
                    log(ERROR, "main: read failed: %s", strerror(errno));
                    break;
                }

                int remaining = readySize;
                char* ptr = buffer;
                while (remaining > 0)
                {
                    // Have we got enough for a Message header?
                    if (remaining < sizeof (Message))
                    {
                        log(ERROR, "Remaining is too small for Message");
                        break;
                    }

                    // Validate the message header
                    Message* msgPtr = (Message*)ptr;
                    if (remaining < msgPtr->size)
                    {
                        log(ERROR, "Remaining is smaller than message size!");
                        break;
                    }

                    void* messageBuffer = calloc(1, 4096);
                    memcpy(messageBuffer, msgPtr, msgPtr->size);
                    remaining -= msgPtr->size;

                    log(DEBUG, "main: Read %d byte message", msgPtr->size);
                    m_connection->receivedResponse((Response*)messageBuffer);
                }

                free(buffer);
        }
#else
        char buffer[4096];
        log(DEBUG, "main: Reading...");
        int res;
        //res = ::read(fd, buffer, 4096);
        res = ::recv(fd, buffer, 4096, 0);
        int err = errno;
        if (res < 0)
        {
            log(WARN, "main: Read %d bytes, err=%s", res, strerror(err));
            break;
        }
        else if (res == 0)
        {
            log(DEBUG, "main: End of file");
            break;
        }
        int remaining = res;
        char* ptr = buffer;
        while (remaining > 0)
        {
            // Have we got enough for a Message header?
            if (remaining < sizeof (Message))
            {
                log(ERROR, "Remaining is too small for Message");
                break;
            }

            // Validate the message header
            Message* msgPtr = (Message*)ptr;
            if (remaining < msgPtr->size)
            {
                log(ERROR, "Remaining is smaller than message size!");
                break;
            }

            void* messageBuffer = calloc(1, 4096);
            memcpy(messageBuffer, msgPtr, msgPtr->size);
            remaining -= msgPtr->size;

            log(DEBUG, "main: Read %d byte message, id=%d", msgPtr->size, msgPtr->id);
            m_connection->receivedResponse((Response*)messageBuffer);
        }

#endif
    }

    m_connection->closed();

    return true;
}

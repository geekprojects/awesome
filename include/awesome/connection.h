#ifndef AWESOME_CONNECTION_H
#define AWESOME_CONNECTION_H

#include <awesome/protocol.h>
#include <geek/core-logger.h>
#include <geek/core-thread.h>

#include <string>
#include <map>
#include <deque>

namespace Awesome
{

class Connection;
class ClientConnection;
class ConnectionSendThread;
class ConnectionReceiveThread;

class ClientSharedMemory
{
 private:
    ClientConnection* m_connection;
    std::string m_path;
    int m_fd;
    int m_size;
    void* m_addr;

 public:
    ClientSharedMemory(ClientConnection* connection, std::string path, int fd, int size, void* addr);
    ~ClientSharedMemory();

    std::string getPath()
    {
        return m_path;
    }

    int getFD() const
    {
        return m_fd;
    }

    int getSize() const
    {
        return m_size;
    }

    void* getAddr() const
    {
        return m_addr;
    }
};

struct ConnectionRequest
{
    Request* request;
    int requestSize;

    Response* response;
    Geek::CondVar* signal;
};

class ConnectionSendThread : public Geek::Thread, Geek::Logger
{
 private:
    Connection* m_connection;
    std::deque<ConnectionRequest*> m_requestQueue;
    Geek::Mutex* m_requestMutex;

    int m_messageId = 0;

    Geek::CondVar* m_requestSignal;

    bool m_running = false;

 public:
    explicit ConnectionSendThread(Connection* connection);
    ~ConnectionSendThread() override;

    bool main() override;

    ConnectionRequest* send(Request* request, int size);
};

class ConnectionReceiveThread : public Geek::Thread, Geek::Logger
{
 private:
    Connection* m_connection;
    bool m_running = false;

 public:
    explicit ConnectionReceiveThread(Connection* connection);
    ~ConnectionReceiveThread();

    bool main() override;
};

class Connection : public Geek::Logger
{
 protected:
    int m_fd = -1;
    std::map<int, ConnectionRequest*> m_requests;

    Geek::Mutex* m_requestMutex;
    ConnectionSendThread* m_connectionThread = nullptr;
    ConnectionReceiveThread* m_connectionReceiveThread = nullptr;

 public:
    Connection();
    virtual ~Connection();

    bool init();

    InfoResponse* getInfo();

    Response* send(Request* message, int size);

    void addRequest(ConnectionRequest* request);
    void receivedResponse(Response* response);

    int getFD() const
    {
        return m_fd;
    }
};

class ClientConnection : public Connection
{
 private:
    std::string m_connectionStr;

    ConnectionSendThread* m_connectionThread;

    bool connectUnix();
    bool connectINet();

 public:
    explicit ClientConnection(std::string connectionStr);
    ~ClientConnection() override;

    bool connect();

    ClientSharedMemory* createSharedMemory(int size);
    void destroySharedMemory(ClientSharedMemory* csm);

    Event* eventPoll();
    Event* eventWait();
};

}

#endif //AWESOME_CONNECTION_H

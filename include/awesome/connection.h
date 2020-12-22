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
    std::string m_path;
    int m_fd;
    int m_size;
    void* m_addr;

 public:
    ClientSharedMemory(std::string path, int fd, int size, void* addr);
    ~ClientSharedMemory();

    std::string getPath()
    {
        return m_path;
    }

    [[nodiscard]] int getSize() const
    {
        return m_size;
    }

    [[nodiscard]] void* getAddr() const
    {
        return m_addr;
    }
};

struct ConnectionRequest
{
    Request* request = nullptr;
    int requestSize;

    Response* response = nullptr;
    Geek::CondVar* signal = nullptr;

    ConnectionRequest()
    {
    }

    ~ConnectionRequest()
    {
        //delete response;
        delete signal;
    }
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

    void stop() { m_running = false;}
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

    void stop() { m_running = false;}
};

class Connection : public Geek::Logger
{
 protected:
    int m_fd = -1;
    std::map<int, ConnectionRequest*> m_requests;

    Geek::Mutex* m_requestMutex;
    ConnectionSendThread* m_connectionSendThread = nullptr;
    ConnectionReceiveThread* m_connectionReceiveThread = nullptr;

    friend ConnectionSendThread;
    friend ConnectionReceiveThread;

 public:
    Connection();
    virtual ~Connection();

    bool init();
    void close();

    [[nodiscard]] bool isOpen() const { return m_fd != -1; }

    InfoResponse* getInfo();

    Response* send(Request* message, int size);

    void addRequest(ConnectionRequest* request);
    void receivedResponse(Response* response);

    [[nodiscard]] int getFD() const
    {
        return m_fd;
    }
};

class ClientConnection : public Connection
{
 private:
    std::string m_connectionStr;

    bool connectUnix();
    bool connectINet();

 public:
    explicit ClientConnection(const std::string& connectionStr);
    ~ClientConnection() override;

    bool connect();

    ClientSharedMemory* createSharedMemory(int size);
    void destroySharedMemory(ClientSharedMemory* csm);

    Event* eventPoll();
    Event* eventWait();
};

}

#endif //AWESOME_CONNECTION_H

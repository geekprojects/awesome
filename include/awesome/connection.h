#ifndef AWESOME_CONNECTION_H
#define AWESOME_CONNECTION_H

#include <string>

#include <awesome/protocol.h>
#include <geek/core-logger.h>

namespace Awesome
{

class ClientConnection;

class ClientSharedMemory
{
 private:
    ClientConnection* m_connection;
    int m_shmid;
    int m_size;
    void* m_addr;

 public:
    ClientSharedMemory(ClientConnection* connection, int shmid, int size, void* addr);
    ~ClientSharedMemory();

    int getShmid() const
    {
        return m_shmid;
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

class Connection : public Geek::Logger
{
 protected:
    int m_fd = -1;
    int m_messageId = 0;

 public:
    Connection();
    virtual ~Connection();

    InfoResponse* getInfo();

    bool send(Message* message, int size);
    Message* wait();
};

class ClientConnection : public Connection
{
 private:
    std::string m_connectionStr;

    bool connectUnix();
    bool connectINet();

 public:
    explicit ClientConnection(std::string connectionStr);
    ~ClientConnection() override;

    bool connect();

    ClientSharedMemory* createSharedMemory(int size);
    void destroySharedMemory(ClientSharedMemory* csm);
};

}

#endif //AWESOME_CONNECTION_H

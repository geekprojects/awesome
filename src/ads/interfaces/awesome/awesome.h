//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_AWESOME_H
#define AWESOME_AWESOME_H

#include <awesome/interface.h>
#include <awesome/protocol.h>
#include <awesome/client.h>

#include <geek/core-thread.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <map>

namespace Awesome
{
class DisplayServer;
class AwesomeServerThread;
class Connection;


class AwesomeInterface : public Interface
{
 private:
    AwesomeServerThread* m_serverThread = nullptr;

    int m_serverSocket = -1;

    struct event_base* m_eventBase = nullptr;
    struct event* m_listenerEvent = nullptr;

    static void acceptCallback(evutil_socket_t listener, short event, void *arg);
    void acceptCallback(short event);


 public:
    explicit AwesomeInterface(DisplayServer* displayServer);
    ~AwesomeInterface() override;

    bool init() override;

    event_base* getEventBase()
    {
        return m_eventBase;
    }
};

class AwesomeClient : public Awesome::Client, Geek::Logger
{
 private:
    bufferevent* m_bufferEvent;

    std::map<int, void*> m_sharedMemory;

    void errorCallback(bufferevent *bev, short error);
    void readCallback(struct bufferevent *bev);

    void handleInfoRequest(InfoRequest* infoRequest);
    void handleSharedMemoryAttach(ShmAttachRequest* request);
    void handleSharedMemoryDetach(ShmDetachRequest* request);
    void handleWindowCreate(WindowCreateRequest* request);
    void handleWindowUpdate(WindowUpdateRequest* request);

 public:
    AwesomeClient(AwesomeInterface* awesomeInterface, bufferevent* bev);
    ~AwesomeClient();

    static void errorCallback(bufferevent *bev, short error, void *ctx);
    static void readCallback(struct bufferevent *bev, void *ctx);
};

class AwesomeServerThread : public Geek::Thread, public Geek::Logger
{
 private:
    AwesomeInterface* m_interface;

 public:
    explicit AwesomeServerThread(AwesomeInterface* interface);
    ~AwesomeServerThread() override;

    bool main() override;
};

}

#endif //AWESOME_AWESOME_H

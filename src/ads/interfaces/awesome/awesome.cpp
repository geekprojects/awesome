
#include "awesome.h"

#include <awesome/displayserver.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <cstdio>
#include <unistd.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

#define MAX_LINE 16384

AwesomeInterface::AwesomeInterface(DisplayServer* displayServer) : Interface("Frontier", displayServer)
{
}

AwesomeInterface::~AwesomeInterface()
{
    // TODO: Do cleanup!
}

bool AwesomeInterface::init()
{
    int res;

    m_eventBase = event_base_new();
    if (m_eventBase == nullptr)
    {
        log(ERROR, "init: Failed to get libevent base");
        return false;
    }

    m_serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(m_serverSocket);
    log(DEBUG, "init: serverSocket=%d", m_serverSocket);

    string socketPath = "/usr/local/var/awesome/ads.socket";

    unlink(socketPath.c_str());

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path)-1);
    res = ::bind(m_serverSocket, (struct sockaddr*)&addr, sizeof(addr));
    if (res != 0)
    {
        log(ERROR, "init: Failed to bind to socket: %s", strerror(errno));
        return false;
    }

    listen(m_serverSocket, 5);

    m_listenerEvent = event_new(m_eventBase, m_serverSocket, EV_READ | EV_PERSIST, acceptCallback, (void*) this);
    event_add(m_listenerEvent, NULL);

    log(DEBUG, "init: Starting server thread...");
    m_serverThread = new AwesomeServerThread(this);
    m_serverThread->start();

    log(DEBUG, "init: Done!");
    return true;
}

void AwesomeInterface::acceptCallback(evutil_socket_t listener, short event, void *arg)
{
    AwesomeInterface* interface = (AwesomeInterface*)arg;
    interface->acceptCallback(event);
}

void AwesomeInterface::acceptCallback(short event)
{
    sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(m_serverSocket, (struct sockaddr*)&ss, &slen);
    log(DEBUG, "acceptCallback: fd=%d", fd);

    if (fd < 0)
    {
        perror("accept");
    }
    else if (fd > FD_SETSIZE)
    {
        close(fd);
    }
    else
    {
        bufferevent* bev;
        evutil_make_socket_nonblocking(fd);
        bev = bufferevent_socket_new(m_eventBase, fd, BEV_OPT_CLOSE_ON_FREE);
        AwesomeClient* client = new AwesomeClient(this, bev);
        bufferevent_setcb(bev, AwesomeClient::readCallback, NULL, AwesomeClient::errorCallback, client);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ|EV_WRITE);

        m_displayServer->addClient(client);
    }
}

AwesomeServerThread::AwesomeServerThread(AwesomeInterface* interface) : Logger("AwesomeServerThread")
{
    m_interface = interface;
}

AwesomeServerThread::~AwesomeServerThread()
{

}

bool AwesomeServerThread::main()
{
    log(DEBUG, "main: Starting libevent dispatch...");
    event_base_dispatch(m_interface->getEventBase());
    log(DEBUG, "main: Done!");
    return true;
}

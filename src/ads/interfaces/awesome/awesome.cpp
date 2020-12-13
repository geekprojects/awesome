
#include "awesome.h"

#include <awesome/displayserver.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <cstdio>
#include <unistd.h>
#include <event2/thread.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

#define MAX_LINE 16384

AwesomeInterface::AwesomeInterface(DisplayServer* displayServer) : Interface("Frontier", displayServer)
{
}

AwesomeInterface::~AwesomeInterface()
{
    event_base_loopexit(m_eventBase, nullptr);

    if (m_serverSocket != -1)
    {
        close(m_serverSocket);
    }
}

static void eventLog(int severity, const char *msg)
{
    const char *s;
    switch (severity) {
        case _EVENT_LOG_DEBUG: s = "debug"; break;
        case _EVENT_LOG_MSG:   s = "msg";   break;
        case _EVENT_LOG_WARN:  s = "warn";  break;
        case _EVENT_LOG_ERR:   s = "error"; break;
        default:               s = "?";     break; /* never reached */
    }
    printf("eventLog: [%s] %s\n", s, msg);
}

void eventFatalCallback(int err)
{
    printf("eventFatalCallback: err=%d\n", err);
}

bool AwesomeInterface::init()
{
    int res;

    event_enable_debug_mode();
    event_set_log_callback(eventLog);
    event_enable_debug_logging(EVENT_DBG_ALL);
    event_set_fatal_callback(eventFatalCallback);

    evthread_enable_lock_debuging();
    evthread_use_pthreads();

    m_eventBase = event_base_new();
    if (m_eventBase == nullptr)
    {
        log(ERROR, "init: Failed to get libevent base");
        return false;
    }

    evthread_make_base_notifiable(m_eventBase);

#if 1
    m_serverSocket = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (m_serverSocket == -1)
    {
        log(ERROR, "init : Failed to create socket: %s", strerror(errno));
        return false;
    }
    //evutil_make_socket_nonblocking(m_serverSocket);
    log(DEBUG, "init: serverSocket=%d", m_serverSocket);

    string socketPath = "/usr/local/var/awesome/ads.socket";

    unlink(socketPath.c_str());

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path)-1);

#else
    m_serverSocket = socket(PF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr;
    addr.sin_port        = htons(16523);
    addr.sin_family      = PF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

#endif

    evutil_make_socket_nonblocking(m_serverSocket);

    res = ::bind(m_serverSocket, (struct sockaddr*)&addr, sizeof(addr));
    if (res != 0)
    {
        log(ERROR, "init: Failed to bind to socket: %s", strerror(errno));
        return false;
    }

    listen(m_serverSocket, 5);

    m_listenerEvent = event_new(m_eventBase, m_serverSocket, EV_READ /*| EV_WRITE*/ | EV_PERSIST, acceptCallback, (void*) this);

    event_add(m_listenerEvent, nullptr);


    log(DEBUG, "init: Starting server thread...");
    m_serverThread = new AwesomeServerThread(this);
    m_serverThread->start();

    log(DEBUG, "init: Done!");
    return true;
}

void AwesomeInterface::acceptCallback(evutil_socket_t listener, short event, void *arg)
{
    ((AwesomeInterface*)arg)->acceptCallback(event);
}

void AwesomeInterface::acceptCallback(short event)
{
    sockaddr ss;
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
        /*
        linger linger;
        memset(&linger, 0, sizeof(struct linger));
        int res;
        res = setsockopt(fd, SOL_SOCKET, SO_LINGER, (const void*)&linger, sizeof(struct linger));
         */

        bufferevent* bev;
        evutil_make_socket_nonblocking(fd);
        bev = bufferevent_socket_new(m_eventBase, fd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);

        AwesomeClient* client = new AwesomeClient(this, fd, bev);
        m_displayServer->addClient(client);
        log(DEBUG, "acceptCallback: New client: %p", client);
        bufferevent_setcb(bev, AwesomeClient::readCallback, NULL, AwesomeClient::errorCallback, client);
        //bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ/*|EV_WRITE*/);
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

#include <awesome/displayserver.h>
#include <awesome/window.h>

#include <csignal>

using namespace Awesome;
using namespace Geek;

DisplayServer* g_displayServer = nullptr;
static void sigintHandler(int sig, siginfo_t* siginfo, void* context)
{
    if (g_displayServer != nullptr)
    {
        g_displayServer->quit();
    }
}

int main(int argc, char** argv)
{
    struct sigaction act;
    memset (&act, 0, sizeof(act));
    act.sa_sigaction = sigintHandler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &act, 0);

    g_displayServer = new DisplayServer();

    int res;
    res = g_displayServer->init();
    if (!res)
    {
        delete g_displayServer;
        return 1;
    }

    g_displayServer->main();

    delete g_displayServer;
}

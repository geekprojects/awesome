#include <awesome/displayserver.h>
#include <awesome/window.h>

using namespace Awesome;
using namespace Geek;

int main(int argc, char** argv)
{
    DisplayServer* displayServer = new DisplayServer();

    int res;
    res = displayServer->init();
    if (!res)
    {
        delete displayServer;
        return 1;
    }

    displayServer->main();

    delete displayServer;
}

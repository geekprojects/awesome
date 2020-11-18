#include <awesome/displayserver.h>
#include <awesome/window.h>

using namespace Awesome;
using namespace Geek;

int main(int argc, char** argv)
{
    DisplayServer* displayServer = new DisplayServer();

    displayServer->init();

    /*
    Window* window1 = new Window();
    window1->setRect(Rect(0, 0, 200, 200));
    displayServer->getCompositor()->addWindow(window1);

    Window* window2 = new Window();
    window2->setRect(Rect(100, 100, 300, 300));
    displayServer->getCompositor()->addWindow(window2);
     */

    displayServer->main();

    delete displayServer;
}

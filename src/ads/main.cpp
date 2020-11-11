#include <awesome/displayserver.h>
#include <awesome/window.h>

using namespace Awesome;
using namespace Geek;

int main(int argc, char** argv)
{
    DisplayServer* displayServer = new DisplayServer();

    displayServer->init();

    Window* window = new Window();
    window->setRect(Rect(100, 100, 200, 200));
    displayServer->getCompositor()->addWindow(window);

    displayServer->main();

    delete displayServer;
}

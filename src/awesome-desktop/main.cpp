//
//

#include <geek/gfx-surface.h>
#include <awesome/connection.h>
#include <cstring>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

int main(int argc, char** argv)
{

    ClientConnection* client = new ClientConnection("unix:/usr/local/var/awesome/ads.socket");
    //ClientConnection* client = new ClientConnection("inet:localhost");
    bool res;
    res = client->connect();
    if (!res)
    {
        return 1;
    }

    InfoResponse* info = client->getInfo();
    printf("ads-test: name=%s, vendor=%s\n", info->name, info->vendor);
    printf("ads-test: numDisplays=%d\n", info->numDisplays);

    Surface* backgroundImage = Surface::loadJPEG("../data/anteater.jpg");

    unsigned int i;
    for (i = 0; i < info->numDisplays; i++)
    {
        InfoDisplayRequest infoDisplayRequest;
        infoDisplayRequest.display = i;
        InfoDisplayResponse* response = static_cast<InfoDisplayResponse*>(client->send(
            &infoDisplayRequest,
            sizeof(infoDisplayRequest)));

        printf("display %u: poa: %d, %d, size: %d, %d\n", i, response->x, response->y, response->width, response->height);

        int scaledWidth = response->width * 2;
        int scaledHeight = response->height * 2;

        ClientSharedMemory* csm = client->createSharedMemory(scaledWidth * scaledHeight * 4);

        float zx = (float)scaledWidth / (float)backgroundImage->getWidth();
        float zy = (float)scaledHeight / (float)backgroundImage->getHeight();
        Surface* resizedBackground = backgroundImage->scale(MAX(zx, zy));

        Surface* surface = new Surface(scaledWidth, scaledHeight, 4, (uint8_t*)csm->getAddr());
        surface->clear(0);
        surface->blit(0, 0, resizedBackground);

        delete resizedBackground;

        WindowCreateRequest createWindowRequest;
        createWindowRequest.width = response->width;
        createWindowRequest.height = response->height;
        createWindowRequest.flags = WINDOW_BACKGROUND;
        wcscpy(createWindowRequest.title, L"Desktop");
        WindowCreateResponse* windowCreateResponse = static_cast<WindowCreateResponse*>(
            client->send(
                &createWindowRequest,
                sizeof(createWindowRequest)));

        printf("ads-test: window id=%d", windowCreateResponse->windowId);

        WindowUpdateRequest windowUpdateRequest;
        windowUpdateRequest.windowId = windowCreateResponse->windowId;
        strncpy(windowUpdateRequest.shmPath, csm->getPath().c_str(), 256);
        client->send(&windowUpdateRequest, sizeof(windowUpdateRequest));

        free(response);
    }
    delete info;

    while (true)
    {
        Event* event = client->eventWait();
        printf("Got event: %p", event);
        if (event == nullptr)
        {
            break;
        }

        delete event;
    }


    return 0;
}

#include <awesome/connection.h>

#include <unistd.h>

using namespace Awesome;
using namespace Geek;
using namespace std;

int main(int argc, char** argv)
{
    ClientConnection* client = new ClientConnection("unix:/usr/local/var/awesome/ads.socket");
    //ClientConnection* client = new ClientConnection("inet:localhost");

    client->connect();

    InfoResponse* info = client->getInfo();
    printf("ads-test: name=%s, vendor=%s\n", info->name, info->vendor);
    delete info;

    unsigned int i;
    for (i = 0; i < info->numDisplays; i++)
    {
        InfoDisplayRequest infoDisplayRequest;
        infoDisplayRequest.display = i;
        InfoDisplayResponse* response = static_cast<InfoDisplayResponse*>(client->send(
            &infoDisplayRequest,
            sizeof(infoDisplayRequest)));

        printf("display %u: %d, %d\n", i, response->width, response->height);

        free(response);
    }

    /*
    ClientSharedMemory* csm = client->createSharedMemory((100 * 2) * (100 * 2) * 4);

    uint32_t p = 0xff888888;
    memset_pattern4(csm->getAddr(), &p, 200 * 200 * 4);

    WindowCreateRequest createWindowRequest;
    createWindowRequest.width = 100;
    createWindowRequest.height = 100;
    WindowCreateResponse* windowCreateResponse = static_cast<WindowCreateResponse*>(
        client->send(
            &createWindowRequest,
            sizeof(createWindowRequest)));

    printf("ads-test: window id=%d", windowCreateResponse->windowId);

    WindowUpdateRequest windowUpdateRequest;
    windowUpdateRequest.windowId = windowCreateResponse->windowId;
    strncpy(windowUpdateRequest.shmPath, csm->getPath().c_str(), 256);
    client->send(&windowUpdateRequest, sizeof(windowUpdateRequest));

    sleep(2);

    client->destroySharedMemory(csm);
    delete csm;
     */

    delete client;

    return 0;
}

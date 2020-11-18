
#include <awesome/connection.h>

#include <unistd.h>

using namespace Awesome;

int main(int argc, char** argv)
{
    ClientConnection* client = new ClientConnection("unix:/usr/local/var/awesome/ads.socket");

    client->connect();

    InfoResponse* info = client->getInfo();
    printf("ads-test: name=%s, vendor=%s", info->name, info->vendor);
    delete info;

    ClientSharedMemory* csm = client->createSharedMemory(100 * 100 * 4);

    uint32_t p = 0xff888888;
    memset_pattern4(csm->getAddr(), &p, 100 * 100 * 4);

    WindowCreateRequest createWindowRequest;
    createWindowRequest.width = 100;
    createWindowRequest.height = 100;
    client->send(&createWindowRequest, sizeof(createWindowRequest));

    WindowCreateResponse* windowCreateResponse = static_cast<WindowCreateResponse*>(client->wait());
    printf("ads-test: window id=%d", windowCreateResponse->windowId);

    WindowUpdateRequest windowUpdateRequest;
    windowUpdateRequest.windowId = windowCreateResponse->windowId;
    windowUpdateRequest.shmid = csm->getShmid();
    client->send(&windowUpdateRequest, sizeof(windowUpdateRequest));

    sleep(2);

    client->destroySharedMemory(csm);
    delete csm;

    delete client;

    return 0;
}

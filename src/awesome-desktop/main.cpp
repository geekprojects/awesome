//
//

#include <geek/gfx-surface.h>
#include <awesome/connection.h>
#include <cstring>
#include <geek/fonts.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

int main(int argc, char** argv)
{
    FontManager* m_fontManager = new FontManager();
    m_fontManager->init();
#if defined(__APPLE__) && defined(__MACH__)
    m_fontManager->scan("/Library/Fonts");
    m_fontManager->scan("/System/Library/Fonts");
    const char* homechar = getenv("HOME");
    m_fontManager->scan(string(homechar) + "/Library/Fonts");
#else
    m_fontManager->scan("/usr/share/fonts");
#endif

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

        int scaledWidth = response->width * response->scale;
        int scaledHeight = response->height * response->scale;

        {
            ClientSharedMemory* csm = client->createSharedMemory(scaledWidth * scaledHeight * 4);

            float zx = (float) scaledWidth / (float) backgroundImage->getWidth();
            float zy = (float) scaledHeight / (float) backgroundImage->getHeight();
            Surface* resizedBackground = backgroundImage->scale(MAX(zx, zy));

            Surface* surface = new Surface(scaledWidth, scaledHeight, 4, (uint8_t*) csm->getAddr());
            surface->clear(0);
            surface->blit(0, 0, resizedBackground);

            delete resizedBackground;

            WindowCreateRequest createWindowRequest;
            createWindowRequest.x = 0;
            createWindowRequest.y = 0;
            createWindowRequest.width = response->width;
            createWindowRequest.height = response->height;
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
            windowUpdateRequest.width = scaledWidth;
            windowUpdateRequest.height = scaledHeight;
            strncpy(windowUpdateRequest.shmPath, csm->getPath().c_str(), 256);
            client->send(&windowUpdateRequest, sizeof(windowUpdateRequest));
        }

        {
            int scaledMenuHeight = 24 * response->scale;
            ClientSharedMemory* csm = client->createSharedMemory(scaledWidth * scaledMenuHeight * 4);

            Surface* menuBarSurface = new Surface(scaledWidth, scaledMenuHeight, 4, (uint8_t*)csm->getAddr());
            menuBarSurface->clear(0xffffffff);

FontHandle* titleFont = m_fontManager->openFont("Helvetica Neue", "Bold", 12 * response->scale);
m_fontManager->write(titleFont, menuBarSurface, 2, 3, L"Awesome", 0x0, true, nullptr);
            FontHandle* menuFont = m_fontManager->openFont("Helvetica Neue", "Regular", 12 * response->scale);
            m_fontManager->write(menuFont, menuBarSurface, 250, 3, L"File", 0x0, true, nullptr);
            m_fontManager->write(menuFont, menuBarSurface, 300, 3, L"Edit", 0x0, true, nullptr);

            WindowCreateRequest createMenuWindowRequest;
            createMenuWindowRequest.x = 0;
            createMenuWindowRequest.y = 0;
            createMenuWindowRequest.width = response->width;
            createMenuWindowRequest.height = 24;
            createMenuWindowRequest.flags = WINDOW_FOREGROUND;
            wcscpy(createMenuWindowRequest.title, L"Awesome Menu");
            WindowCreateResponse* menuWindowCreateResponse = static_cast<WindowCreateResponse*>(
                client->send(
                    &createMenuWindowRequest,
                    sizeof(createMenuWindowRequest)));

            printf("ads-test: menu window id=%d", menuWindowCreateResponse->windowId);

            WindowUpdateRequest windowUpdateRequest;
            windowUpdateRequest.windowId = menuWindowCreateResponse->windowId;
            windowUpdateRequest.width = menuBarSurface->getWidth();
            windowUpdateRequest.height = menuBarSurface->getHeight();
            strncpy(windowUpdateRequest.shmPath, csm->getPath().c_str(), 256);
            client->send(&windowUpdateRequest, sizeof(windowUpdateRequest));
        }

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
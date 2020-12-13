
#include "linuxdrm.h"

#include <awesome/displayserver.h>

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;

DRMDisplayDriver::DRMDisplayDriver(DisplayServer* displayServer) : DisplayDriver("DRM", displayServer)
{
}

DRMDisplayDriver::~DRMDisplayDriver()
{

}

bool DRMDisplayDriver::init()
{
    m_drmFD = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    log(DEBUG, "init: fd=%d", m_drmFD);

    if (m_drmFD < 0)
    {
        log(ERROR, "init: Failed to find graphics device");
        return false;
    }

    int res;
    uint64_t hasDumb;
    res = drmGetCap(m_drmFD, DRM_CAP_DUMB_BUFFER, &hasDumb);
    log(DEBUG, "init: hasDumb=%d", hasDumb);
    if (res < 0 || !hasDumb)
    {
        log(ERROR, "DRM device does not support dumb buffers");
        return false;
    }

    drmModeRes* modeRes;
    modeRes = drmModeGetResources(m_drmFD);
    if (modeRes == nullptr)
    {
        log(ERROR, "Unable to get DRM mode resources");
        return false;
    }
    int i;
    log(DEBUG, "init: Found %d CRTCs", modeRes->count_crtcs);
    /*
    for (i = 0; i < modeRes->count_crtcs; i++)
    {
        m_availableCrtcs.insert(i);
    }
     */

    log(DEBUG, "init: Found %d connectors", modeRes->count_connectors);
    for (i = 0; i < modeRes->count_connectors; i++)
    {
        drmModeConnector* conn;
        conn = drmModeGetConnector(m_drmFD, modeRes->connectors[i]);
        log(DEBUG, "init: Connector %d: %p: %d", i, conn, conn->connection);

        if (conn->connection != DRM_MODE_CONNECTED || conn->count_modes == 0)
        {
            continue;
        }

        DRMDisplay* display = new DRMDisplay(this);
        res = display->init(modeRes, conn);
        if (!res)
        {
            continue;
        }

        m_displayServer->addDisplay(display);
    }

    return true;
}

bool DRMDisplayDriver::poll()
{
    sleep(1);
    return true;
}

DRMDisplay::DRMDisplay(DRMDisplayDriver* displayDriver) : Display("DRMDisplay", displayDriver)
{
}

DRMDisplay::~DRMDisplay()
{

}

bool DRMDisplay::init(drmModeRes* modeRes, drmModeConnector* conn)
{
    DRMDisplayDriver* displayDriver = (DRMDisplayDriver*)m_displayDriver;

    m_connector = conn->connector_id;
    m_mode = conn->modes[0];

    m_rect.w = m_mode.hdisplay;
    m_rect.h = m_mode.vdisplay;

    log(DEBUG, "init: Connector %d, %d", m_rect.w, m_rect.h);

    int i;
    for (i = 0; i < conn->count_encoders; i++)
    {
       drmModeEncoder* enc = drmModeGetEncoder(displayDriver->getFd(), conn->encoders[i]);
       if (enc == nullptr)
       {
           continue;
       }

       int j;
       for (j = 0; j < modeRes->count_crtcs; j++)
       {
           if (!(enc->possible_crtcs & (1 << j)))
           {
               continue;
           }

           int c = modeRes->crtcs[j];
           if (displayDriver->getAvailableCrtcs().count(c) == 0)
           {
               m_crtc = c;
               displayDriver->getAvailableCrtcs().insert(c);
               break;
           }
       }
       drmModeFreeEncoder(enc);
    }

    if (m_crtc == -1)
    {
        log(ERROR, "init: Failed to find CRTC");
        return false;
    }
    log(DEBUG, "init: m_crtc=%d", m_crtc);



    drm_mode_create_dumb creq;

    memset(&creq, 0, sizeof(creq));
    creq.width = m_rect.w;
    creq.height = m_rect.h;
    creq.bpp = 32;
    int res;
    res = drmIoctl(displayDriver->getFd(), DRM_IOCTL_MODE_CREATE_DUMB, &creq);
    if (res < 0)
    {
        log(ERROR, "init: Cannot create dumb buffer: %s", strerror(errno));
        return false;
    }

    m_stride = creq.pitch;
    m_size = creq.size;
    m_handle = creq.handle;
    log(DEBUG, "init: stride=%d", m_stride);

    res = drmModeAddFB(
        displayDriver->getFd(),
        m_rect.w,
        m_rect.h,
        24,
        32,
        m_stride,
        m_handle,
        &m_fb);
    if (res < 0)
    {
        log(ERROR, "init: Cannot create frame buffer: %s", strerror(errno));
        return false;
    }
    log(DEBUG, "init: fb=%d", m_fb);

    drm_mode_map_dumb mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.handle = m_handle;
    res = drmIoctl(displayDriver->getFd(), DRM_IOCTL_MODE_MAP_DUMB, &mreq);
    if (res < 0)
    {
        log(ERROR, "init: cannot map dumb buffer: %s", strerror(errno));
    }

    m_map = static_cast<uint8_t*>(mmap(
        0,
        m_size,
        PROT_READ | PROT_WRITE, MAP_SHARED,
        displayDriver->getFd(),
        mreq.offset));
    if (m_map == MAP_FAILED)
    {
        log(ERROR, "init: Failed to mmap buffer: %s", strerror(errno));
        return false;
    }

    memset(m_map, 0x80, m_size);

    drmModeGetCrtc(displayDriver->getFd(), m_crtc);

    res = drmModeSetCrtc(displayDriver->getFd(), m_crtc, m_fb, 0, 0, &(m_connector), 1, &(m_mode));
    if (res < 0)
    {
        log(ERROR, "init: Failed to set CRTC for connector: %s", strerror(errno));
        return false;
    }

    m_fbSurface = new Surface(m_rect.w, m_rect.h, 4, m_map);

    m_fbSurface->clear(0xff00ff00);

    return true;
}

bool DRMDisplay::draw(Window* window, Geek::Rect drawRect)
{
    log(DEBUG, "draw: window=%d", window->getId());
    Rect windowRect = window->getRect();

    if (window->getFrameSurface() != nullptr)
    {
        m_fbSurface->blit(windowRect.x, windowRect.y, window->getFrameSurface());
    }

    DRMWindowDisplayData* data = getDisplayData(window);
    if (data->contentSurface != nullptr)
    {
        Rect rect = window->getContentRect();
        rect.x += windowRect.x;
        rect.y += windowRect.y;
        m_fbSurface->blit(rect.x, rect.y, data->contentSurface);
    }
}

DRMWindowDisplayData* DRMDisplay::getDisplayData(Window* window)
{
    DRMWindowDisplayData* data = dynamic_cast<DRMWindowDisplayData*>(window->getWindowDisplayData(this));
    if (data == nullptr)
    {
        data = new DRMWindowDisplayData();
        window->setWindowDisplayData(this, data);
    }
    return data;
}

void DRMDisplay::update(Window* window, Geek::Gfx::Surface* surface)
{
    DRMWindowDisplayData* data = getDisplayData(window);

    if (data->contentSurface != nullptr)
    {
        delete data->contentSurface;
    }

    data->contentSurface = new Surface(surface);
}

bool DRMDisplay::startDraw()
{
    log(DEBUG, "startDraw: Here!");
    return true;
}

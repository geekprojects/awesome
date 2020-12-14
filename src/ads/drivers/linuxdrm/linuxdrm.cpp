
#include "linuxdrm.h"

#include <awesome/displayserver.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <GL/glu.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;

GLfloat rotTri = 0, rotQuad = 0;

static uint32_t find_crtc_for_encoder(const drmModeRes *resources,
                                      const drmModeEncoder *encoder) {
    int i;

    for (i = 0; i < resources->count_crtcs; i++) {
        /* possible_crtcs is a bitmask as described here:
         * https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
         */
        const uint32_t crtc_mask = 1 << i;
        const uint32_t crtc_id = resources->crtcs[i];
        if (encoder->possible_crtcs & crtc_mask) {
            return crtc_id;
        }
    }

    /* no match found */
    return -1;
}

static uint32_t find_crtc_for_connector(int fd, const drmModeRes *resources,
                                        const drmModeConnector *connector) {
    int i;

    for (i = 0; i < connector->count_encoders; i++) {
        const uint32_t encoder_id = connector->encoders[i];
        drmModeEncoder *encoder = drmModeGetEncoder(fd, encoder_id);

        if (encoder) {
            const uint32_t crtc_id = find_crtc_for_encoder(resources, encoder);

            drmModeFreeEncoder(encoder);
            if (crtc_id != 0) {
                return crtc_id;
            }
        }
    }

    /* no match found */
    return -1;
}

DRMDisplayDriver::DRMDisplayDriver(DisplayServer* displayServer) : OpenGLDisplayDriver("DRM", displayServer)
{
}

DRMDisplayDriver::~DRMDisplayDriver() = default;

bool DRMDisplayDriver::init()
{
    m_drmFD = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    log(DEBUG, "init: fd=%d", m_drmFD);

    if (m_drmFD < 0)
    {
        log(ERROR, "init: Failed to find graphics device");
        return false;
    }

    m_gbmDev = gbm_create_device(m_drmFD);
    log(DEBUG, "init: m_gbmDev=%p", m_gbmDev);

    int res;
    /*
    uint64_t hasDumb;
    res = drmGetCap(m_drmFD, DRM_CAP_DUMB_BUFFER, &hasDumb);
    log(DEBUG, "init: hasDumb=%d", hasDumb);
    if (res < 0 || !hasDumb)
    {
        log(ERROR, "DRM device does not support dumb buffers");
        return false;
    }
     */

    drmModeRes* modeRes;
    modeRes = drmModeGetResources(m_drmFD);
    if (modeRes == nullptr)
    {
        log(ERROR, "Unable to get DRM mode resources");
        return false;
    }
    int i;
    log(DEBUG, "init: Found %d CRTCs", modeRes->count_crtcs);

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

DRMDisplay::DRMDisplay(DRMDisplayDriver* displayDriver) : OpenGLDisplay("DRMDisplay", displayDriver)
{
}

DRMDisplay::~DRMDisplay() = default;

bool DRMDisplay::init(drmModeRes* modeRes, drmModeConnector* conn)
{
    DRMDisplayDriver* displayDriver = (DRMDisplayDriver*)m_displayDriver;

    m_connector = conn->connector_id;
    m_mode = conn->modes[0];

    int i;
    int area = 0;
    for (i = 0, area = 0; i < conn->count_modes; i++) {
        drmModeModeInfo *current_mode = &conn->modes[i];

        if (current_mode->type & DRM_MODE_TYPE_PREFERRED) {
            m_mode = *current_mode;
        }

        int current_area = current_mode->hdisplay * current_mode->vdisplay;
        if (current_area > area) {
            m_mode = *current_mode;
            area = current_area;
        }
    }

    m_rect.w = m_mode.hdisplay;
    m_rect.h = m_mode.vdisplay;

    log(DEBUG, "init: Connector %d, %d", m_rect.w, m_rect.h);

    m_crtc = -1;
    drmModeEncoder* enc = nullptr;
    for (i = 0; i < modeRes->count_encoders; i++) {
        enc = drmModeGetEncoder(displayDriver->getFd(), modeRes->encoders[i]);
        if (enc->encoder_id == conn->encoder_id)
            break;
        drmModeFreeEncoder(enc);
        enc = NULL;
    }

    if (false && enc != nullptr)
    {
        m_crtc = enc->crtc_id;
        displayDriver->getAvailableCrtcs().insert(m_crtc);
    }
    else
    {
#if 1
        //int i;
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
#endif
        //m_crtc = find_crtc_for_connector(displayDriver->getFd(), modeRes, conn);
    }

    if (m_crtc == -1)
    {
        log(ERROR, "init: Failed to find CRTC");
        return false;
    }
    log(DEBUG, "init: m_crtc=%d", m_crtc);

    m_gbmSurface = gbm_surface_create(
        displayDriver->getGbmDev(),
        m_mode.hdisplay, m_mode.vdisplay,
        GBM_FORMAT_XRGB8888,
        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (m_gbmSurface == nullptr)
    {
        log(ERROR, "init: Failed to create gbm surface!");
        return false;
    }

    EGLint major, minor, n;
    GLint ret;

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    static const EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display;
    get_platform_display = (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (get_platform_display == nullptr)
    {
        log(ERROR, "init: Failed to get eglGetPlatformDisplayEXT");
        return false;
    }

    m_eglDisplay = get_platform_display(EGL_PLATFORM_GBM_KHR, displayDriver->getGbmDev(), nullptr);
    int res;
    res = eglInitialize(m_eglDisplay, &major, &minor);
    if (!res)
    {
        log(ERROR, "Failed to initialize EGL");
        return false;
    }

    log(DEBUG, "init: Using display %p with EGL version %d.%d",
           m_eglDisplay, major, minor);

    log(DEBUG, "EGL Version \"%s\"", eglQueryString(m_eglDisplay, EGL_VERSION));
    log(DEBUG, "EGL Vendor \"%s\"", eglQueryString(m_eglDisplay, EGL_VENDOR));
    log(DEBUG, "EGL Extensions \"%s\"", eglQueryString(m_eglDisplay, EGL_EXTENSIONS));

    res = eglBindAPI(EGL_OPENGL_API);
    if (!res)
    {
        log(ERROR, "init: failed to bind api EGL_OPENGL_API");
        return false;
    }

    res = eglChooseConfig(m_eglDisplay, config_attribs, &m_eglConfig, 1, &n);
    if (!res || n != 1)
    {
        log(ERROR, "init: failed to choose config: %d", n);
        return false;
    }

    m_eglContext = eglCreateContext(
        m_eglDisplay,
        m_eglConfig,
        EGL_NO_CONTEXT,
        context_attribs);
    if (m_eglContext == nullptr)
    {
        log(ERROR, "init: failed to create context");
        return false;
    }

    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, m_gbmSurface, nullptr);
    if (m_eglSurface == EGL_NO_SURFACE)
    {
        log(ERROR, "init: failed to create egl surface");
        return false;
    }

    /* connect the context to the surface */
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);

    log(INFO, "init: GL Extensions: \"%s\"", glGetString(GL_EXTENSIONS));

    glClearColor(0.5, 0.5, 0.5, 1.0);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH);
    eglSwapBuffers(m_eglDisplay, m_eglSurface);

    m_gbmBo = gbm_surface_lock_front_buffer(m_gbmSurface);
    DRMBOData* fb = drmFbGetFromBo(m_gbmBo);

    ret = drmModeSetCrtc(
        displayDriver->getFd(),
        m_crtc,
        fb->fb_id,
        0,
        0,
        &m_connector,
        1,
        &m_mode);
    if (ret)
    {
        log(ERROR, "init: failed to set mode: %s", strerror(errno));
        return ret;
    }

    OpenGLDisplay::init();

    glFlush();

    releaseCurrentContext();

    return true;
}

static void drm_fb_destroy_callback(gbm_bo *bo, void *data)
{
    printf("drm_fb_destroy_callback: bo=%p\n", bo);
    DRMBOData* fb = static_cast<DRMBOData*>(data);
    gbm_device *gbm = gbm_bo_get_device(bo);

    if (fb->fb_id)
    {
        drmModeRmFB(fb->displayDriver->getFd(), fb->fb_id);
    }

    free(fb);
}

DRMBOData* DRMDisplay::drmFbGetFromBo(gbm_bo* bo)
{
    DRMBOData* fb = (DRMBOData*)gbm_bo_get_user_data(bo);
    uint32_t width, height, stride, handle;
    int ret;

    if (fb != nullptr)
    {
        return fb;
    }

    fb = static_cast<DRMBOData*>(calloc(1, sizeof *fb));
    fb->bo = bo;

    width = gbm_bo_get_width(bo);
    height = gbm_bo_get_height(bo);
    stride = gbm_bo_get_stride(bo);
    handle = gbm_bo_get_handle(bo).u32;

    ret = drmModeAddFB(((DRMDisplayDriver*)m_displayDriver)->getFd(), width, height, 24, 32, stride, handle, &fb->fb_id);
    if (ret)
    {
        printf("failed to create fb: %s\n", strerror(errno));
        free(fb);
        return NULL;
    }
    fb->displayDriver = dynamic_cast<DRMDisplayDriver*>(m_displayDriver);

    gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

    return fb;
}

void DRMDisplay::setCurrentContext()
{
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext);
}

static void page_flip_handler(
    int fd,
    unsigned int frame,
    unsigned int sec,
    unsigned int usec,
    void *data)
{
    int *waiting_for_flip = (int*)data;
    *waiting_for_flip = 0;
}

void DRMDisplay::swapBuffers()
{
    log(DEBUG, "swapBuffers: Swapping.. (display=%p, surface=%p)", m_eglDisplay, m_eglSurface);
    int res;
    eglSwapBuffers(m_eglDisplay, m_eglSurface);

    gbm_bo* next_bo = gbm_surface_lock_front_buffer(m_gbmSurface);
    log(DEBUG, "swapBuffers: Next buffer: %p", next_bo);
    if (next_bo == nullptr)
    {
        return;
    }
    DRMBOData* fb = drmFbGetFromBo(next_bo);

    /*
     * Here you could also update drm plane layers if you want
     * hw composition
     */

    int waiting_for_flip = 1;
    int fd = ((DRMDisplayDriver*)m_displayDriver)->getFd();
    while (true)
    {
        res = drmModePageFlip(
            fd,
            m_crtc,
            fb->fb_id,
            DRM_MODE_PAGE_FLIP_EVENT,
            &waiting_for_flip);
        if (res)
        {
            int err = errno;
            if (err == EBUSY)
            {
                // Try again!
            }
            else
            {
                log(ERROR, "swapBuffers: failed to queue page flip: %s (%d)", strerror(err), err);
                return;
            }
        }
        else
        {
            break;
        }
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(fd, &fds);

    while (waiting_for_flip)
    {
        res = select(fd + 1, &fds, nullptr, nullptr, nullptr);
        if (res < 0)
        {
            log(ERROR, "select err: %s", strerror(errno));
            return;
        }
        else if (res == 0)
        {
            log(ERROR, "select timeout!");
            return;
        }
        else if (FD_ISSET(0, &fds))
        {
            printf("user interrupted!\n");
            break;
        }

        drmEventContext evctx =
        {
            .version = DRM_EVENT_CONTEXT_VERSION,
            .page_flip_handler = page_flip_handler,
        };
        drmHandleEvent(fd, &evctx);
    }

    /* release last buffer to render on again: */
    gbm_surface_release_buffer(m_gbmSurface, m_gbmBo);
    m_gbmBo = next_bo;

}

void DRMDisplay::releaseCurrentContext()
{
    eglMakeCurrent(m_eglDisplay,  EGL_NO_SURFACE,  EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

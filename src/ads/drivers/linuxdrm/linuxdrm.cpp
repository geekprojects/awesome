
#include "linuxdrm.h"

#include <awesome/displayserver.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <GL/glu.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;

REGISTER_DISPLAY_DRIVER(DRM)

DRMDisplayDriver::DRMDisplayDriver(DisplayServer* displayServer) : OpenGLDisplayDriver("DRM", displayServer)
{
}

DRMDisplayDriver::~DRMDisplayDriver()
{
    if (m_libInput != nullptr)
    {
        libinput_unref(m_libInput);
    }
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

    m_gbmDev = gbm_create_device(m_drmFD);
    log(DEBUG, "init: m_gbmDev=%p, backend=%s", m_gbmDev, gbm_device_get_backend_name(m_gbmDev));

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
            drmModeFreeConnector(conn);
            continue;
        }

        DRMDisplay* display = new DRMDisplay(this);
        int res = display->init(modeRes, conn);
        drmModeFreeConnector(conn);
        if (!res)
        {
            continue;
        }

        m_displayServer->addDisplay(display);
    }

    return initInput();
}

static int open_restricted(const char *path, int flags, void *user_data)
{
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data)
{
    close(fd);
}

const static struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

bool DRMDisplayDriver::initInput()
{
    struct udev *udev = udev_new();
    m_libInput = libinput_udev_create_context(&interface, NULL, udev);
    log(DEBUG, "initInput: m_libInput=%p", m_libInput);

    int res;
    res = libinput_udev_assign_seat(m_libInput, "seat0");
    log(DEBUG, "initInput: assign_seat res=%d", res);

    return true;
}

bool DRMDisplayDriver::poll()
{
    int fd = libinput_get_fd(m_libInput);

    fd_set fdSet;
    FD_ZERO (&fdSet);
    FD_SET (fd, &fdSet);

    int res;
    res = select(FD_SETSIZE, &fdSet, nullptr, nullptr, nullptr);
    if (res <= 0)
    {
        return false;
    }

    while (true)
    {
        libinput_dispatch(m_libInput);
        libinput_event* event;
        event = libinput_get_event(m_libInput);
        if (event == nullptr)
        {
            break;
        }

        libinput_event_type type = libinput_event_get_type(event);

        switch (type)
        {
            case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
            {
                libinput_event_pointer* p = libinput_event_get_pointer_event(event);
                double x = libinput_event_pointer_get_absolute_x_transformed(p, m_displayServer->getTotalWidth());
                double y = libinput_event_pointer_get_absolute_y_transformed(p, m_displayServer->getTotalHeight());

                Event* mouseEvent = new Event();
                mouseEvent->eventType = AWESOME_EVENT_MOUSE_MOTION;
                mouseEvent->mouse.x = (int)x;
                mouseEvent->mouse.y = (int)y;

                m_displayServer->getCompositor()->postEvent(mouseEvent);
            } break;

            default:
                log(DEBUG, "poll: Unhandled type=%d", type);
                break;
        }

        libinput_event_destroy(event);
    }

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH);
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
        return nullptr;
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
    int res;
    eglSwapBuffers(m_eglDisplay, m_eglSurface);

    gbm_bo* next_bo = gbm_surface_lock_front_buffer(m_gbmSurface);
    if (next_bo == nullptr)
    {
        log(ERROR, "swapBuffers: Failed to get next buffer object");
        return;
    }
    DRMBOData* fb = drmFbGetFromBo(next_bo);

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

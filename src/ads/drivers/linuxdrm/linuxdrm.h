#ifndef AWESOME_DRIVERS_LINUXDRM_H
#define AWESOME_DRIVERS_LINUXDRM_H

#include <awesome/displaydriver.h>
#include <awesome/display.h>

#include "drivers/opengl.h"

#include <geek/core-thread.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <set>

namespace Awesome
{

class DRMDisplayDriver;

struct DRMBOData
{
    DRMDisplayDriver* displayDriver;
    struct gbm_bo* bo;
    uint32_t fb_id;
};

class DRMDisplay : public OpenGLDisplay
{
 private:
    drmModeModeInfo m_mode;
    int m_crtc = -1;
    uint32_t m_connector;

    gbm_surface* m_gbmSurface;
    gbm_bo* m_gbmBo;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    EGLContext m_eglContext;
    EGLSurface m_eglSurface;

    DRMBOData* drmFbGetFromBo(gbm_bo *bo);

 protected:
    void setCurrentContext() override;
    void releaseCurrentContext() override;
    void swapBuffers() override;

 public:
    explicit DRMDisplay(DRMDisplayDriver* displayDriver);
    ~DRMDisplay() override;

    bool init(drmModeRes* modeRes, drmModeConnector* connector);


    /*
    void update(Window* window, Geek::Gfx::Surface* surface) override;
     */
};

class DRMDisplayDriver : public OpenGLDisplayDriver
{
 private:
    int m_drmFD = -1;
    std::set<int> m_availableCrtcs;
    gbm_device* m_gbmDev{};

    void initDisplay(drmModeConnector* conn);

 public:
    explicit DRMDisplayDriver(DisplayServer* displayServer);
    ~DRMDisplayDriver() override;

    bool init() override;

    bool poll() override;

    int getFd() { return m_drmFD; }

    std::set<int>& getAvailableCrtcs() { return m_availableCrtcs; }
    gbm_device* getGbmDev() { return m_gbmDev; }
};

}

#endif //AWESOME_DRIVERS_LINUXDRM_H

#ifndef AWESOME_DRIVERS_LINUXDRM_H
#define AWESOME_DRIVERS_LINUXDRM_H

#include <awesome/displaydriver.h>
#include <awesome/display.h>

#include <geek/core-thread.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <set>

namespace Awesome
{

class DRMDisplayDriver;

struct DRMWindowDisplayData : WindowDisplayData
{
    Geek::Mutex* mutex = nullptr;

    Geek::Gfx::Surface* contentSurface = nullptr;

    DRMWindowDisplayData() {}
    ~DRMWindowDisplayData() override {}

};

class DRMDisplay : public Display
{
 private:
    drmModeModeInfo m_mode;
    int m_crtc = -1;

    uint32_t m_connector;
    uint32_t m_stride;
    uint32_t m_size;
    uint32_t m_handle;
    uint8_t* m_map;
    uint32_t m_fb;

    Geek::Gfx::Surface* m_fbSurface;

    DRMWindowDisplayData* getDisplayData(Window* window);

 public:
    DRMDisplay(DRMDisplayDriver* displayDriver);
    ~DRMDisplay();

    bool init(drmModeRes* modeRes, drmModeConnector* connector);

    bool startDraw() override;

    bool draw(Window* window, Geek::Rect drawRect) override;

    void update(Window* window, Geek::Gfx::Surface* surface) override;
};

class DRMDisplayDriver : public DisplayDriver
{
 private:
    int m_drmFD = -1;
    std::set<int> m_availableCrtcs;

    void initDisplay(drmModeConnector* conn);

 public:
    explicit DRMDisplayDriver(DisplayServer* displayServer);
    ~DRMDisplayDriver() override;

    bool init() override;

    bool poll() override;

    int getFd() { return m_drmFD; }

    std::set<int>& getAvailableCrtcs() { return m_availableCrtcs; }
};

}

#endif //AWESOME_DRIVERS_LINUXDRM_H

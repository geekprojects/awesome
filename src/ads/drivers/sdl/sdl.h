//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_DRIVERS_SDL_H
#define AWESOME_DRIVERS_SDL_H

#include <awesome/displaydriver.h>
#include <awesome/display.h>

#include <geek/core-thread.h>

#include <map>

#include <SDL.h>
#include <SDL_video.h>
#include <awesome/event.h>
#include <yaml-cpp/yaml.h>

#include "drivers/opengl.h"

namespace Awesome
{
class SDLDisplay;

class SDLDisplayDriver : public OpenGLDisplayDriver
{
 private:
    std::map<SDL_Window*, SDLDisplay*> m_displays;
    std::map<uint32_t, uint32_t> m_keycodeTable;

    Event* m_keyDownEvent = nullptr;
    std::string m_lastText;

 public:
    explicit SDLDisplayDriver(DisplayServer* displayServer);
    ~SDLDisplayDriver() override;

    bool init() override;
    void quit() override;

    bool poll() override;
};

class SDLDisplay : public OpenGLDisplay
{
 private:
    YAML::Node m_config;
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

 protected:
    void setCurrentContext() override;
    void releaseCurrentContext() override;
    void swapBuffers() override;

 public:
    SDLDisplay(SDLDisplayDriver* driver, YAML::Node& config);
    ~SDLDisplay() override;

    bool init() override;

    SDL_Window* getWindow() const
    {
        return m_window;
    }
};

}

#endif //AWESOME_GL_H

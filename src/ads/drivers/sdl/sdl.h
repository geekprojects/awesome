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

#include "drivers/opengl.h"

namespace Awesome
{
class SDLDisplay;

class SDLDisplayDriver : public OpenGLDisplayDriver
{
 private:
    std::map<SDL_Window*, SDLDisplay*> m_displays;

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
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

 protected:
    void setCurrentContext() override;
    void releaseCurrentContext() override;
    void swapBuffers() override;

 public:
    explicit SDLDisplay(SDLDisplayDriver* driver);
    ~SDLDisplay() override;

    bool init() override;

    //bool startDraw() override;
    //bool draw(Window* window, Geek::Rect drawRect) override;
    //void endDraw() override;

    SDL_Window* getWindow() const
    {
        return m_window;
    }
};

}

#endif //AWESOME_GL_H

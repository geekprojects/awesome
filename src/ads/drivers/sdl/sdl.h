//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_GL_H
#define AWESOME_GL_H

#include <awesome/displaydriver.h>
#include <awesome/display.h>

#include <geek/core-thread.h>

#include <map>

#include <SDL.h>
#include <SDL_video.h>

#if defined(__APPLE__) || defined(MACOSX)
#define GL_SILENCE_DEPRECATION
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 1
#include <OpenGL/OpenGLAvailability.h>
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#include <GL/gl3.h>
#endif

namespace Awesome
{
class SDLDisplay;

class SDLDisplayDriver : public DisplayDriver
{
 private:
    std::map<SDL_Window*, SDLDisplay*> m_displays;

 public:
    explicit SDLDisplayDriver(DisplayServer* displayServer);
    ~SDLDisplayDriver() override;

    bool init() override;

    bool poll() override;
};

struct SDLWindowDisplayData : WindowDisplayData
{
    Geek::Mutex* mutex = nullptr;
    bool valid = false;
    unsigned int texture = 0;
    unsigned int sampler = 0;
    float textureCoordX = 0;
    float textureCoordY = 0;
    Geek::Gfx::Surface* textureSurface = nullptr;

    SDLWindowDisplayData() {}
    ~SDLWindowDisplayData() override {}

};

class SDLDisplay : public Display
{
 private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    SDLWindowDisplayData* getData(Window* window);
    void updateTexture(Window* window, SDLWindowDisplayData* data, Geek::Gfx::Surface* surface);

 public:
    explicit SDLDisplay(SDLDisplayDriver* driver);
    ~SDLDisplay() override;

    bool init() override;

    bool startDraw() override;
    bool draw(Window* window, Geek::Rect drawRect) override;
    void endDraw() override;

    void update(Window* window, Geek::Gfx::Surface* surface) override;

    SDL_Window* getWindow() const
    {
        return m_window;
    }
};

}

#endif //AWESOME_GL_H

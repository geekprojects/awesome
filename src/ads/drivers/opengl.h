//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_DRIVERS_OPENGL_H
#define AWESOME_DRIVERS_OPENGL_H

#include <awesome/displaydriver.h>
#include <awesome/display.h>

#include <geek/core-thread.h>

#include <map>

#if defined(__APPLE__) || defined(MACOSX)
#define GL_SILENCE_DEPRECATION
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 1
#include <OpenGL/OpenGLAvailability.h>
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
//#include <GL/gl3.h>
#endif

namespace Awesome
{
class OpenGLDisplay;

class OpenGLDisplayDriver : public DisplayDriver
{
 private:
 public:
    explicit OpenGLDisplayDriver(const std::string& name, DisplayServer* displayServer);
    ~OpenGLDisplayDriver() override;
};

class OpenGLTexture
{
 private:
    bool valid = false;
    unsigned int texture = 0;
    // unsigned int sampler = 0;
    float coordX = 0;
    float coordY = 0;
    Geek::Gfx::Surface* surface = nullptr;

 public:
    OpenGLTexture();
    ~OpenGLTexture();

    void update(Geek::Gfx::Surface* surface);
    void draw(Geek::Rect rect, float scale);
};

struct OpenGLWindowDisplayData : WindowDisplayData
{
    Geek::Mutex* mutex = nullptr;

    OpenGLTexture* frameTexture;
    OpenGLTexture* contentTexture;

    OpenGLWindowDisplayData() = default;
    ~OpenGLWindowDisplayData() override = default;
};

class OpenGLDisplay : public Display
{
 private:
    Cursor* m_currentCursor = nullptr;
    OpenGLTexture* m_cursorTexture = nullptr;

 protected:
    OpenGLWindowDisplayData* getData(Window* window);
    void updateTexture(OpenGLWindowDisplayData* data, OpenGLTexture* texture, Geek::Gfx::Surface* surface);

    virtual void setCurrentContext() = 0;
    virtual void releaseCurrentContext() = 0;
    virtual void swapBuffers() = 0;

 public:
    explicit OpenGLDisplay(const std::string& name, OpenGLDisplayDriver* driver);
    ~OpenGLDisplay() override;

    bool init() override;

    bool startDraw() override;
    bool draw(Window* window, Geek::Rect drawRect) override;
    void endDraw() override;

    void drawCursor(Cursor* cursor, Geek::Vector2D pos) override;

    void update(Window* window, Geek::Gfx::Surface* surface) override;
    void updateFrame(Window* window) override;
};

}

#endif //AWESOME_GL_H

//
// Created by Ian Parker on 05/11/2020.
//

#include "sdl.h"
#include <awesome/displayserver.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;

static int powerOfTwo(int input)
{
    int value = 1;

    while (value < input)
    {
        value <<= 1;
    }
    return value;
}

SDLDisplayDriver::SDLDisplayDriver(DisplayServer* displayServer) : DisplayDriver("SDL", displayServer)
{
}

SDLDisplayDriver::~SDLDisplayDriver()
{
}

bool SDLDisplayDriver::init()
{
    int res;
    res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (res < 0)
    {
        log(ERROR, "init: Failed to initialise SDL!");
        return false;
    }

    SDLDisplay* display = new SDLDisplay(this);
    display->init();
    m_displayServer->addDisplay(display);

    return true;
}

bool SDLDisplayDriver::poll()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                log(INFO, "checkEvent: SDL_QUIT!");
                return false;
            case SDL_WINDOWEVENT:
            {
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_CLOSE:
                        log(INFO, "checkEvent: SDL_WINDOWEVENT_CLOSE!");
                        return false;
                }
            } break;

        }
    }

    return true;
}

SDLDisplay::SDLDisplay(SDLDisplayDriver* displayDriver) : Display("SDLDisplay", displayDriver)
{

}

SDLDisplay::~SDLDisplay()
{

}

bool SDLDisplay::init()
{
    m_rect.w = 1024;
    m_rect.h = 768;

    int flags =
        SDL_WINDOW_OPENGL |
        SDL_WINDOW_SHOWN |
        SDL_WINDOW_ALLOW_HIGHDPI ;

    log(DEBUG, "init: Creating window...");
    m_window = SDL_CreateWindow(
        "Awesome SDL Display",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        m_rect.w,
        m_rect.h,
        flags);
    log(DEBUG, "init: m_window=%p", m_window);
    if (m_window == NULL)
    {
        log( ERROR, "open: Failed to open window: %s", SDL_GetError());
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);

    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    return true;
}

static void makeIdentity(double m[16])
{
    memset(m, 0, sizeof(double) * 16);
    m[0+4*0] = 1;
    m[1+4*1] = 1;
    m[2+4*2] = 1;
    m[3+4*3] = 1;
}

static void setPerspective(double fovy, double aspect, double zNear, double zFar)
{
    double radians = fovy / 2 * M_PI / 180;

    double deltaZ = zFar - zNear;
    double sine = sin(radians);
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
    {
        return;
    }
    double cotangent = cos(radians) / sine;

    double m[4][4];
    makeIdentity(&m[0][0]);
    m[0][0] = cotangent / aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1;
    m[3][2] = -2 * zNear * zFar / deltaZ;
    m[3][3] = 0;
    glMultMatrixd(&m[0][0]);
}

void SDLDisplay::resize()
{
    GLfloat ratio;
    ratio = ( GLfloat )m_rect.w / ( GLfloat )m_rect.h;

    // Setup our viewport
    glViewport( 0, 0, ( GLsizei )m_rect.w, ( GLsizei )m_rect.h );

    // change to the projection matrix and set our viewing volume
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );

    // Set our perspective
    setPerspective( 45.0f, ratio, 0.001f, 1000.0f );

    // Make sure we're chaning the model view and not the projection
    glMatrixMode( GL_MODELVIEW );

    // Reset The View
    glLoadIdentity( );

    glFlush();
}

bool SDLDisplay::startDraw()
{
    glMatrixMode(GL_MODELVIEW);
    // clear the drawing buffer.
    glClear(GL_COLOR_BUFFER_BIT);
    // clear the identity matrix.
    glLoadIdentity();

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    //glDisable(GL_TEXTURE_2D);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    /* This allows alpha blending of 2D textures with the scene */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);

    float scale = 2.0;

    glViewport(0, 0, m_rect.w * scale, m_rect.h * scale);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, (double)m_rect.w * scale, (double)m_rect.h * scale, 0.0, 0.0, 1.0);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    return true;
}

bool SDLDisplay::draw(Window* window, Geek::Rect drawRect)
{
    SDLWindowDisplayData* data = getData(window);
    Rect windowRect = window->getRect();

    if (!data->valid)
    {
        log(DEBUG, "draw: Creating default surface");
        Surface* surface = new Surface(windowRect.w, windowRect.h, 4);
        surface->clear(0);
        updateTexture(window, data, surface);
        delete surface;
    }

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, data->texture);
    glBegin(GL_TRIANGLE_STRIP);

    float scale = 2.0f;

    glTexCoord2f(0, 0);
    glVertex2i(windowRect.x, windowRect.y);

    glTexCoord2f(data->textureCoordX, 0);
    glVertex2i(windowRect.x + (windowRect.w * scale), windowRect.y);

    glTexCoord2f(0, data->textureCoordY);
    glVertex2i(windowRect.x, windowRect.y + (windowRect.h * scale));

    glTexCoord2f(data->textureCoordX, data->textureCoordY);
    glVertex2i(windowRect.x + (windowRect.w * scale), windowRect.y + (windowRect.h * scale));

    glEnd();

    return true;
}

void SDLDisplay::endDraw()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();

    glFlush();

    SDL_GL_SwapWindow(m_window);
}

void SDLDisplay::update(Window* window, Geek::Gfx::Surface* surface)
{
    updateTexture(window, getData(window), surface);
}

SDLWindowDisplayData* SDLDisplay::getData(Window* window)
{
    auto data = (SDLWindowDisplayData*)window->getWindowDisplayData(this);
    if (data == nullptr)
    {
        data = new SDLWindowDisplayData();
        window->setWindowDisplayData(this, data);
    }

    return data;
}

void SDLDisplay::updateTexture(Window* window, SDLWindowDisplayData* data, Geek::Gfx::Surface* surface)
{
    log(DEBUG, "updateTexture: window=%p, data=%p", window, data);
    int winWidth = window->getRect().w;
    int winHeight = window->getRect().h;

    float scale = 1;

    unsigned int textureWidth = powerOfTwo(winWidth * scale);
    unsigned int textureHeight = powerOfTwo(winHeight * scale);
    data->textureCoordX = (float)(winWidth * scale) / (float)textureWidth;
    data->textureCoordY = (float)(winHeight * scale) / (float)textureHeight;

    if (data->textureSurface == NULL || data->textureSurface->getWidth() != textureWidth || data->textureSurface->getHeight() != textureHeight )
    {
        if (data->textureSurface != NULL)
        {
            delete data->textureSurface;
        }
        data->textureSurface = new Surface(textureWidth, textureHeight, 4);
        data->textureSurface->clear(0);
    }
    data->textureSurface->blit(0, 0, surface);

    SDL_GL_MakeCurrent(m_window, m_glContext);

    if (data->texture == 0)
    {
        glGenTextures(1, &(data->texture));
    }

    glBindTexture(GL_TEXTURE_2D, data->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        data->textureSurface->getWidth(), data->textureSurface->getHeight(),
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        data->textureSurface->getData());

    data->valid = true;
}

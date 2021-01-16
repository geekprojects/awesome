//
// Created by Ian Parker on 05/11/2020.
//

#include "sdl.h"
#include <awesome/displayserver.h>

using namespace Awesome;
using namespace Geek;
using namespace Geek::Gfx;
using namespace std;

REGISTER_DISPLAY_DRIVER(SDL)

static unsigned int powerOfTwo(int input)
{
    unsigned int value = 1;

    while (value < input)
    {
        value <<= 1u;
    }
    return value;
}

SDLDisplayDriver::SDLDisplayDriver(DisplayServer* displayServer) : OpenGLDisplayDriver("SDL", displayServer)
{
}

SDLDisplayDriver::~SDLDisplayDriver()
{
    SDL_Quit();
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

    auto driverConfig = m_displayServer->getConfig()["drivers"]["display"]["SDL"];
    if (!driverConfig || !driverConfig["displays"])
    {
        // No config, just create a default display
        driverConfig = YAML::Node();
        YAML::Node display;
        display["x"] = 0;
        display["y"] = 0;
        display["width"] = 1024;
        display["height"] = 768;
        driverConfig["displays"][0] = display;
    }

    auto displays = driverConfig["displays"];
    for (auto displayConfig : displays)
    {
        SDLDisplay* display = new SDLDisplay(this, displayConfig);
        display->init();

        m_displays.insert(make_pair(display->getWindow(), display));
        m_displayServer->addDisplay(display);
    }


    int i;
    for (i = 32; i < 128; i++)
    {
        m_keycodeTable.insert(make_pair(i, toupper(i)));
    }
    m_keycodeTable.insert(make_pair(SDLK_BACKSPACE, KC_BACKSPACE));
    m_keycodeTable.insert(make_pair(SDLK_TAB, KC_TAB));
    //m_keycodeTable.insert(make_pair(, KC_LINEFEED));
    m_keycodeTable.insert(make_pair(SDLK_CLEAR, KC_CLEAR));
    m_keycodeTable.insert(make_pair(SDLK_RETURN, KC_RETURN));
    m_keycodeTable.insert(make_pair(SDLK_PAUSE, KC_PAUSE));
    m_keycodeTable.insert(make_pair(SDLK_SCROLLLOCK, KC_SCROLL_LOCK));
    m_keycodeTable.insert(make_pair(SDLK_SYSREQ, KC_SYS_REQ));
    m_keycodeTable.insert(make_pair(SDLK_ESCAPE, KC_ESCAPE));
    m_keycodeTable.insert(make_pair(SDLK_DELETE, KC_DELETE));
    m_keycodeTable.insert(make_pair(SDLK_HOME, KC_HOME));
    m_keycodeTable.insert(make_pair(SDLK_LEFT, KC_LEFT));
    m_keycodeTable.insert(make_pair(SDLK_UP, KC_UP));
    m_keycodeTable.insert(make_pair(SDLK_RIGHT, KC_RIGHT));
    m_keycodeTable.insert(make_pair(SDLK_DOWN, KC_DOWN));
    m_keycodeTable.insert(make_pair(SDLK_PRIOR, KC_PRIOR));
    m_keycodeTable.insert(make_pair(SDLK_PAGEUP, KC_PAGE_UP));
    //m_keycodeTable.insert(make_pair(, KC_NEXT));
    m_keycodeTable.insert(make_pair(SDLK_PAGEDOWN, KC_PAGE_DOWN));
    m_keycodeTable.insert(make_pair(SDLK_END, KC_END));
    //m_keycodeTable.insert(make_pair(, KC_BEGIN));
    m_keycodeTable.insert(make_pair(SDLK_SELECT, KC_SELECT));
    m_keycodeTable.insert(make_pair(SDLK_PRINTSCREEN, KC_PRINT_SCREEN));
    m_keycodeTable.insert(make_pair(SDLK_EXECUTE, KC_EXECUTE));
    m_keycodeTable.insert(make_pair(SDLK_INSERT, KC_INSERT));
    m_keycodeTable.insert(make_pair(SDLK_UNDO, KC_UNDO));
    //m_keycodeTable.insert(make_pair(, KC_REDO));
    //m_keycodeTable.insert(make_pair(, KC_MENU));
    m_keycodeTable.insert(make_pair(SDLK_FIND, KC_FIND));
    m_keycodeTable.insert(make_pair(SDLK_CANCEL, KC_CANCEL));
    m_keycodeTable.insert(make_pair(SDLK_HELP, KC_HELP));
    //m_keycodeTable.insert(make_pair(, KC_BREAK));
    m_keycodeTable.insert(make_pair(SDLK_MODE, KC_MODE_SWITCH));
    //m_keycodeTable.insert(make_pair(, KC_SCRIPT_SWITCH));
    //m_keycodeTable.insert(make_pair(, KC_NUM_LOCK));

    m_keycodeTable.insert(make_pair(SDLK_KP_SPACE, KC_KP_SPACE));
    m_keycodeTable.insert(make_pair(SDLK_KP_TAB, KC_KP_TAB));
    m_keycodeTable.insert(make_pair(SDLK_KP_ENTER, KC_KP_ENTER));
    //m_keycodeTable.insert(make_pair(, KC_KP_F1));
    //m_keycodeTable.insert(make_pair(, KC_KP_F2));
    //m_keycodeTable.insert(make_pair(, KC_KP_F3));
    //m_keycodeTable.insert(make_pair(, KC_KP_F4));
    //m_keycodeTable.insert(make_pair(, KC_KP_HOME;
    //m_keycodeTable.insert(make_pair(, KC_KP_LEFT));
    //m_keycodeTable.insert(make_pair(, KC_KP_UP));
    //m_keycodeTable.insert(make_pair(, KC_KP_RIGHT));
    //m_keycodeTable.insert(make_pair(, KC_KP_DOWN));
    //m_keycodeTable.insert(make_pair(, KC_KP_PRIOR));
    //m_keycodeTable.insert(make_pair(, KC_KP_PAGE_UP));
    //m_keycodeTable.insert(make_pair(, KC_KP_NEXT));
    //m_keycodeTable.insert(make_pair(, KC_KP_PAGE_DOWN));
    //m_keycodeTable.insert(make_pair(, KC_KP_END));
    //m_keycodeTable.insert(make_pair(, KC_KP_BEGIN));
    //m_keycodeTable.insert(make_pair(, KC_KP_INSERT));
    //m_keycodeTable.insert(make_pair(, KC_KP_DELETE));
    m_keycodeTable.insert(make_pair(SDLK_KP_EQUALS, KC_KP_EQUAL));
    m_keycodeTable.insert(make_pair(SDLK_KP_MULTIPLY, KC_KP_MULTIPLY));
    m_keycodeTable.insert(make_pair(SDLK_KP_PLUS, KC_KP_ADD));
    m_keycodeTable.insert(make_pair(SDLK_KP_COMMA, KC_KP_COMMA));
    m_keycodeTable.insert(make_pair(SDLK_KP_MINUS, KC_KP_SUBTRACT));
    m_keycodeTable.insert(make_pair(SDLK_KP_DECIMAL, KC_KP_DECIMAL));
    m_keycodeTable.insert(make_pair(SDLK_KP_DIVIDE, KC_KP_DIVIDE));
    m_keycodeTable.insert(make_pair(SDLK_KP_0, KC_KP_0));
    m_keycodeTable.insert(make_pair(SDLK_KP_1, KC_KP_1));
    m_keycodeTable.insert(make_pair(SDLK_KP_2, KC_KP_2));
    m_keycodeTable.insert(make_pair(SDLK_KP_3, KC_KP_3));
    m_keycodeTable.insert(make_pair(SDLK_KP_4, KC_KP_4));
    m_keycodeTable.insert(make_pair(SDLK_KP_5, KC_KP_5));
    m_keycodeTable.insert(make_pair(SDLK_KP_6, KC_KP_6));
    m_keycodeTable.insert(make_pair(SDLK_KP_7, KC_KP_7));
    m_keycodeTable.insert(make_pair(SDLK_KP_8, KC_KP_8));
    m_keycodeTable.insert(make_pair(SDLK_KP_8, KC_KP_9));

    m_keycodeTable.insert(make_pair(SDLK_F1, KC_F1));
    m_keycodeTable.insert(make_pair(SDLK_F2, KC_F2));
    m_keycodeTable.insert(make_pair(SDLK_F3, KC_F3));
    m_keycodeTable.insert(make_pair(SDLK_F4, KC_F4));
    m_keycodeTable.insert(make_pair(SDLK_F5, KC_F5));
    m_keycodeTable.insert(make_pair(SDLK_F6, KC_F6));
    m_keycodeTable.insert(make_pair(SDLK_F7, KC_F7));
    m_keycodeTable.insert(make_pair(SDLK_F8, KC_F8));
    m_keycodeTable.insert(make_pair(SDLK_F9, KC_F9));
    m_keycodeTable.insert(make_pair(SDLK_F10, KC_F10));
    m_keycodeTable.insert(make_pair(SDLK_F11, KC_F11));
    m_keycodeTable.insert(make_pair(SDLK_F12, KC_F12));

    return true;
}

bool SDLDisplayDriver::poll()
{
    SDL_Event event;
    log(DEBUG, "poll: Waiting for events...");
    while (SDL_WaitEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                log(INFO, "checkEvent: SDL_QUIT!");
                return false;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                Event* keyEvent = new Event();
                keyEvent->eventType = AWESOME_EVENT_KEY;
                keyEvent->key.direction = (event.type == SDL_KEYDOWN);

                auto it = m_keycodeTable.find(event.key.keysym.sym);
                if (it != m_keycodeTable.end())
                {
                    keyEvent->key.key = it->second;
                    if (!(event.key.keysym.sym & SDLK_SCANCODE_MASK) && iswprint(event.key.keysym.sym))
                    {
                        keyEvent->key.chr = event.key.keysym.sym;
                    }

                    keyEvent->key.modifiers = 0;
                    if (event.key.keysym.mod & KMOD_LSHIFT)
                    {
                        keyEvent->key.modifiers |= KeyModifier::KMOD_SHIFT_L;
                    }
                    if (event.key.keysym.mod & KMOD_RSHIFT)
                    {
                        keyEvent->key.modifiers |= KeyModifier::KMOD_SHIFT_R;
                    }

                    if (keyEvent->key.modifiers & (KeyModifier::KMOD_SHIFT_L | KeyModifier::KMOD_SHIFT_R))
                    {
                        keyEvent->key.chr = toupper(keyEvent->key.chr);
                    }

                    if (event.type == SDL_KEYUP || (keyEvent->key.chr == 0))
                    {
                        m_displayServer->getCompositor()->postEvent(keyEvent);
                        m_lastText = "";
                    }
                    else if (event.type == SDL_KEYDOWN)
                    {
                        m_keyDownEvent = keyEvent;
                    }
                }
            } break;
            case SDL_TEXTINPUT:
            {
                if (m_keyDownEvent != NULL)
                {
                    log(DEBUG,
                        "poll: SDL_TEXTINPUT: Sending DOWN: chr=%s, key=0x%x, mod=0x%x",
                        event.text.text,
                        m_keyDownEvent->key.key,
                        m_keyDownEvent->key.modifiers);
                    m_lastText = string(event.text.text);

                    Event* eventCopy = new Event();
                    *eventCopy = *m_keyDownEvent;
                    if (m_lastText.length() > 0)
                    {
                        eventCopy->key.chr = m_lastText.at(0);
                    }

                    m_displayServer->getCompositor()->postEvent(eventCopy);
                    m_keyDownEvent = NULL;
                }
                else
                {
                    log(DEBUG, "poll: SDL_TEXTINPUT: m_keyDownEvent is NULL");
                }
            } break;

            case SDL_MOUSEMOTION:
            {
                SDL_Window* sdlWindow = SDL_GetWindowFromID(event.button.windowID);
                auto it = m_displays.find(sdlWindow);
                if (it != m_displays.end())
                {
                    SDLDisplay* display = it->second;
                    Event* mouseEvent = new Event();
                    mouseEvent->eventType = AWESOME_EVENT_MOUSE_MOTION;
                    mouseEvent->mouse.x = event.button.x + display->getRect().x;
                    mouseEvent->mouse.y = event.button.y + display->getRect().y;

                    m_displayServer->getCompositor()->postEvent(mouseEvent);
                }
            } break;

            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
            {
                SDL_Window* sdlWindow = SDL_GetWindowFromID(event.button.windowID);
                auto it = m_displays.find(sdlWindow);
                if (it != m_displays.end())
                {
                    SDLDisplay* display = it->second;
                    Event* mouseEvent = new Event();
                    mouseEvent->eventType = AWESOME_EVENT_MOUSE_BUTTON;
                    mouseEvent->mouse.x = event.button.x + display->getRect().x;
                    mouseEvent->mouse.y = event.button.y + display->getRect().y;
                    mouseEvent->mouse.button.buttons = 1;
                    mouseEvent->mouse.button.direction = (event.type == SDL_MOUSEBUTTONDOWN);

                    m_displayServer->getCompositor()->postEvent(mouseEvent);
                }
            } break;

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

void SDLDisplayDriver::quit()
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = SDL_QUIT;

    SDL_PushEvent(&event);
}

SDLDisplay::SDLDisplay(SDLDisplayDriver* displayDriver, YAML::Node& config) : OpenGLDisplay("SDLDisplay", displayDriver)
{
    m_config = config;
}

SDLDisplay::~SDLDisplay()
{
    if (m_glContext != nullptr)
    {
        SDL_GL_DeleteContext(m_glContext);
    }
    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
    }
}

bool SDLDisplay::init()
{
    m_rect.w = m_config["width"].as<int>();
    m_rect.h = m_config["height"].as<int>();

    unsigned int flags =
        SDL_WINDOW_OPENGL |
        SDL_WINDOW_SHOWN |
        SDL_WINDOW_ALLOW_HIGHDPI;

    log(DEBUG, "init: Creating window...");
    m_window = SDL_CreateWindow(
        "Awesome SDL Display",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        m_rect.w,
        m_rect.h,
        flags);
    log(DEBUG, "init: m_window=%p", m_window);
    if (m_window == nullptr)
    {
        log( ERROR, "open: Failed to open window: %s", SDL_GetError());
        return false;
    }

    SDL_ShowCursor(false);

    int drawableWidth;
    int drawableHeight;
    SDL_GL_GetDrawableSize(m_window, &drawableWidth, &drawableHeight);
    m_scale = (float)drawableWidth / m_rect.w;
    log(DEBUG, "init: window: %d, %d  drawable: %d, %d  scale=%0.2f", m_rect.w, m_rect.h, drawableWidth, drawableHeight, m_scale);

    m_glContext = SDL_GL_CreateContext(m_window);

    return OpenGLDisplay::init();
}

void SDLDisplay::setCurrentContext()
{
    SDL_GL_MakeCurrent(m_window, m_glContext);
}

void SDLDisplay::releaseCurrentContext()
{
}

void SDLDisplay::swapBuffers()
{
    SDL_GL_SwapWindow(m_window);
}

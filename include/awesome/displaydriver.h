//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_DISPLAYDRIVER_H
#define AWESOME_DISPLAYDRIVER_H

#include <geek/core-logger.h>

#include <map>

namespace Awesome
{

class DisplayServer;

class DisplayDriver : public Geek::Logger
{
 protected:
    std::string m_name;
    DisplayServer* m_displayServer;

 public:
    DisplayDriver(const std::string& name, DisplayServer* displayServer);
    virtual ~DisplayDriver();

    virtual bool init();
    virtual bool poll();
    virtual void quit();
};

class DisplayDriverInit
{
 private:
 public:
    DisplayDriverInit() = default;
    virtual ~DisplayDriverInit() = default;

    virtual DisplayDriver* create(DisplayServer* displayServer) { return NULL; }

    virtual const std::string getName() { return ""; }
};

class DisplayDriverRegistry
{
 private:
    std::map<std::string, DisplayDriverInit*> m_widgets;

 public:
    DisplayDriverRegistry();
    virtual ~DisplayDriverRegistry();

    static void registerDisplayDriver(DisplayDriverInit* init);
    static DisplayDriver* createDisplayDriver(DisplayServer* displayServer, const std::string& name);
};

#define REGISTER_DISPLAY_DRIVER(_name)  \
    class _name##DisplayDriverInit : public Awesome::DisplayDriverInit \
    { \
     private: \
        static _name##DisplayDriverInit* const init __attribute__ ((unused)); \
     public: \
        _name##DisplayDriverInit() \
        { \
            DisplayDriverRegistry::registerDisplayDriver(this); \
        } \
        const std::string getName() \
        { \
            return #_name; \
        } \
        _name##DisplayDriver* create(DisplayServer* ds) \
        { \
            return new _name##DisplayDriver(ds); \
        } \
    }; \
    _name##DisplayDriverInit* const _name##DisplayDriverInit::init = new _name##DisplayDriverInit();
}

#endif //AWESOME_DISPLAYDRIVER_H

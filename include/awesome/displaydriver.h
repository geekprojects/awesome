//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_DISPLAYDRIVER_H
#define AWESOME_DISPLAYDRIVER_H

#include <geek/core-logger.h>

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

}

#endif //AWESOME_DISPLAYDRIVER_H

//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_INTERFACE_H
#define AWESOME_INTERFACE_H

#include <geek/core-logger.h>

namespace Awesome
{

class DisplayServer;

class Interface : public Geek::Logger
{
 protected:
    [[maybe_unused]] std::string m_name;
    DisplayServer* m_displayServer;

 public:
    Interface(const std::string& name, DisplayServer* displayServer);
    virtual ~Interface();

    virtual bool init();

    [[nodiscard]] DisplayServer* getDisplayServer() const
    {
        return m_displayServer;
    }
};

}

#endif //AWESOME_INTERFACE_H

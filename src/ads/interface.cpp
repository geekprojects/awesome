//
// Created by Ian Parker on 05/11/2020.
//

#include <awesome/interface.h>

using namespace Awesome;

Interface::Interface(std::string name, DisplayServer* displayServer): Logger("Interface[" + name + "]")
{
    m_name = name;
    m_displayServer = displayServer;
}

Interface::~Interface()
{
}

bool Interface::init()
{
    return true;
}

//
// Created by Ian Parker on 05/11/2020.
//

#include <awesome/displaydriver.h>

using namespace Awesome;

DisplayDriver::DisplayDriver(std::string name, DisplayServer* displayServer)
 : Logger("DisplayDriver[" + name + "]")
{
    m_name = name;
    m_displayServer = displayServer;
}

DisplayDriver::~DisplayDriver()
{
}

bool DisplayDriver::init()
{
    return false;
}

bool DisplayDriver::poll()
{
    return false;
}



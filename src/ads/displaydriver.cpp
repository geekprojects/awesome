//
// Created by Ian Parker on 05/11/2020.
//

#include <awesome/displaydriver.h>

using namespace Awesome;

DisplayDriver::DisplayDriver(const std::string& name, DisplayServer* displayServer)
 : Logger("DisplayDriver[" + name + "]")
{
    m_name = name;
    m_displayServer = displayServer;
}

DisplayDriver::~DisplayDriver() = default;

bool DisplayDriver::init()
{
    return false;
}

bool DisplayDriver::poll()
{
    return false;
}

void DisplayDriver::quit()
{
}

static DisplayDriverRegistry* g_displayServerRegistry = nullptr;

DisplayDriverRegistry::DisplayDriverRegistry() = default;

DisplayDriverRegistry::~DisplayDriverRegistry() = default;

void DisplayDriverRegistry::registerDisplayDriver(DisplayDriverInit* init)
{
    if (g_displayServerRegistry == nullptr)
    {
        g_displayServerRegistry = new DisplayDriverRegistry();
    }
    printf("DisplayDriverRegistry::registerDisplayDriver: %s\n", init->getName().c_str());
    g_displayServerRegistry->m_widgets.insert(make_pair(init->getName(), init));
}

DisplayDriver* DisplayDriverRegistry::createDisplayDriver(DisplayServer* displayServer, const std::string& name)
{
    auto it = g_displayServerRegistry->m_widgets.find(name);
    if (it != g_displayServerRegistry->m_widgets.end())
    {
        DisplayDriverInit* init = it->second;
        return init->create(displayServer);
    }
    return nullptr;
}

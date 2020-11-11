
#include <awesome/interfaces/frontier.h>

using namespace Awesome;

FrontierInterface::FrontierInterface(DisplayServer* displayServer) : Interface("Frontier", displayServer)
{
}

FrontierInterface::~FrontierInterface()
{

}

bool FrontierInterface::init()
{
    return true;
}

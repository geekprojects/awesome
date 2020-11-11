//
// Created by Ian Parker on 05/11/2020.
//

#ifndef AWESOME_FRONTIER_H
#define AWESOME_FRONTIER_H

#include <awesome/interface.h>

namespace Awesome
{
class DisplayServer;

class FrontierInterface : public Interface
{
 private:
 public:
    explicit FrontierInterface(DisplayServer* displayServer);
    ~FrontierInterface() override;

    bool init() override;
};
}

#endif //AWESOME_FRONTIER_H

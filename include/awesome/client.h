//
// Created by Ian Parker on 17/11/2020.
//

#ifndef AWESOME_CLIENT_H
#define AWESOME_CLIENT_H

namespace Awesome
{
class Interface;

class Client
{
 protected:
    Interface* m_interface;

 public:
    Client(Interface* interface);
    ~Client();

};

}

#endif //AWESOME_CLIENT_H

//
// Created by Ian Parker on 17/11/2020.
//

#ifndef AWESOME_CLIENT_H
#define AWESOME_CLIENT_H

#include <awesome/event.h>
#include <geek/core-thread.h>

#include <deque>

namespace Awesome
{
class Interface;

class Client
{
 protected:
    Interface* m_interface;

    Geek::Mutex* m_eventsMutex;
    Geek::CondVar* m_eventsSignal;
    std::deque<Event*> m_events;

 public:
    Client(Interface* interface);
    ~Client();

    void postEvent(Event* event);
    Event* popEvent();
    Event* waitEvent();
};

}

#endif //AWESOME_CLIENT_H

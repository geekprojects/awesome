
#include <awesome/client.h>

using namespace Awesome;
using namespace Geek;

Client::Client(Interface* interface)
{
    m_interface = interface;
    m_eventsMutex = Thread::createMutex();
    m_eventsSignal = Thread::createCondVar();
}

Client::~Client()
{
}

void Client::postEvent(Event* event)
{
    m_eventsMutex->lock();
    m_events.push_back(event);
    m_eventsMutex->unlock();
    m_eventsSignal->signal();
}

Event* Client::popEvent()
{
    Event* event = nullptr;
    m_eventsMutex->lock();
    if (!m_events.empty())
    {
        event = m_events.front();
        m_events.pop_front();
    }
    m_eventsMutex->unlock();
    return event;
}

Event* Client::waitEvent()
{
    while (true)
    {
        Event* event = popEvent();
        if (event != nullptr)
        {
            return event;
        }

        printf("Client::waitEvent: Waiting for event signal!\n");
        m_eventsSignal->wait();
        printf("Client::waitEvent: Got signal!\n");
    }
}


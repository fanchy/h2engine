#include "base/event_bus.h"
using namespace ff;

EventBus& EventBus::fireEvent(EventBase& event){
    std::vector<EventListener*>& destList = m_allRegCallback[event.eventID()];
    for (size_t i = 0; i < destList.size(); ++i){
        destList[i]->onEvent(event);
    }
    for (size_t i = 0; i < m_allListenAny.size(); ++i){
        m_allListenAny[i]->onEvent(event);
    }
    return *this;
}

EventBus::~EventBus(){
    std::map<int, std::vector<EventListener*> >::iterator it = m_allRegCallback.begin();
    for (; it != m_allRegCallback.end(); ++it){
        std::vector<EventListener*>& destList = it->second;
        for (size_t i = 0; i < destList.size(); ++i){
            delete destList[i];
        }
    }
    m_allRegCallback.clear();
    
    for (size_t i = 0; i < m_allListenAny.size(); ++i){
        delete m_allListenAny[i];
    }
    m_allListenAny.clear();
}

namespace ff{
EventBus& EVENT_BUS_LISTEN(void (*func)(EventBase&)){
    return Singleton<EventBus>::instance().listenAnyEevnt(func);
}
}

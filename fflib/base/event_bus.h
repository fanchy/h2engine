#ifndef _EVENT_BUSS_H_
#define _EVENT_BUSS_H_

#include <string>
#include <vector>

#include "base/singleton.h"
#include "base/fftype.h"

namespace ff{

class EventBase{
public:
    virtual ~EventBase(){}
    virtual int eventID()                  = 0;
    virtual const std::string& eventName() = 0;
};
template<typename T>
class Event:public EventBase{
public:
    virtual int eventID()                   { return TYPEID(T);     }
    virtual const std::string& eventName()  { return TYPE_NAME(T);  }
};

class EventListener{
public:
    virtual ~EventListener(){}
    virtual void onEvent(EventBase& e) = 0;
};

template<typename T>
class EventListenerFunc:public EventListener{
public:
    typedef void (*eventCB)(T&);
    EventListenerFunc(eventCB f):func(f){}
    virtual void onEvent(EventBase& e){
        T* p = (T*)(&e);
        func(*p);
    }
private:
    eventCB func;
};
template<typename T, typename R>
class EventListenerMethod:public EventListener{
public:
    typedef void (R::*eventCB)(T&);
    
    EventListenerMethod(eventCB f, R* o):func(f), obj(o){}
    virtual void onEvent(EventBase& e){
        T* p = (T*)(&e);
        (obj->*func)(*p);
    }
private:
    eventCB func;
    R*      obj;
};

class EventBus
{
public:
    EventBus& fireEvent(EventBase& event);
    ~EventBus();

    template<typename T>
    EventBus& listenEevnt(void (*func)(T&)){
        m_allRegCallback[TYPEID(T)].push_back(new EventListenerFunc<T>(func));
        return *this;
    }
    template<typename T, typename R>
    EventBus& listenEevnt(void (R::*func)(T&), R* obj){
        m_allRegCallback[TYPEID(T)].push_back(new EventListenerMethod<T, R>(func, obj));
        return *this;
    }

    EventBus& listenAnyEevnt(void (*func)(EventBase&)){
        m_allListenAny.push_back(new EventListenerFunc<EventBase>(func));
        return *this;
    }
    template<typename R>
    EventBus& listenAnyEevnt(void (R::*func)(EventBase&), R* obj){
        m_allListenAny.push_back(new EventListenerMethod<EventBase, R>(func, obj));
        return *this;
    }
protected:
    std::map<int, std::vector<EventListener*> >    m_allRegCallback;
    std::vector<EventListener*>                    m_allListenAny;
};

#define EVENT_BUS Singleton<EventBus>::instance()
#define EVENT_BUS_FIRE(X) Singleton<EventBus>::instance().fireEvent(X)

EventBus& EVENT_BUS_LISTEN(void (*func)(EventBase&));
template<typename R>
EventBus& EVENT_BUS_LISTEN(void (R::*func)(EventBase&), R* obj){
    return Singleton<EventBus>::instance().listenAnyEevnt(func, obj);
}

template<typename T>
EventBus& EVENT_BUS_LISTEN(T t){
    return Singleton<EventBus>::instance().listenEevnt(t);
}
template<typename T, typename R>
EventBus& EVENT_BUS_LISTEN(T t, R r){
    return Singleton<EventBus>::instance().listenEevnt(t, r);
}

}
#endif

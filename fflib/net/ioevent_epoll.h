#ifdef linux

#ifndef _IO_EVENT_EPOLL_H_
#define _IO_EVENT_EPOLL_H_

#include "base/func.h"
#include "base/osdef.h"
#include "net/ioevent.h"
#include "base/lock.h"
#include "net/socket_op.h"

#include <map>
#include <vector>
#include <list>

#include <sys/epoll.h>

namespace ff {

#define EPOLL_EVENTS_SIZE  128
class IOEventEpoll: public IOEvent
{
public:
    struct IOInfo
    {
        IOInfo():bBroken(false), bAccept(false){}
        bool    bBroken;
        bool    bAccept;
        IOEventFunc eventHandler;
        std::list<std::string> sendBuffer;
    };
    struct IOCallBackInfo
    {
        Socketfd     fd;
        IOEVENT_TYPE    eventType;
        IOEventFunc     eventHandler;
        std::string     data;
    };
public:
    IOEventEpoll();
    virtual ~IOEventEpoll();

    virtual int run();
    virtual int runOnce(int ms);
    virtual int stop();
    virtual int regfd(Socketfd fd, IOEventFunc eventHandler);
    virtual int regfdAccept(Socketfd fd, IOEventFunc eventHandler);
    virtual int unregfd(Socketfd fd);
    virtual void asyncSend(Socketfd fd, const char* data, size_t len);

    void notify();
    virtual void post(FuncToRun func);
private:
    void safeClosefd(Socketfd fd);
protected:
    volatile bool                   m_running;
    Mutex                           m_mutex;
    std::map<Socketfd, IOInfo>   m_allIOinfo;
    int                             m_efd;
    Socketfd                     m_fdNotify[2];
    std::vector<FuncToRun>          m_listFuncToRun;
    struct epoll_event              ev_set[EPOLL_EVENTS_SIZE];
};

}
#endif
#endif

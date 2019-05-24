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

namespace ff {

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
        SOCKET_TYPE     fd;
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
    virtual int regfd(SOCKET_TYPE fd, IOEventFunc eventHandler);
    virtual int regfdAccept(SOCKET_TYPE fd, IOEventFunc eventHandler);
    virtual int unregfd(SOCKET_TYPE fd);
    virtual void asyncSend(SOCKET_TYPE fd, const char* data, size_t len);

    void notify();
    virtual void post(FuncToRun func);
private:
    void safeClosefd(SOCKET_TYPE fd);
protected:
    volatile bool                   m_running;
    Mutex                           m_mutex;
    std::map<SOCKET_TYPE, IOInfo>   m_allIOinfo;
    int                             m_efd;
    SOCKET_TYPE                     m_fdNotify[2];
    std::vector<FuncToRun>          m_listFuncToRun;
};

}
#endif

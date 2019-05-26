#ifndef _IO_EVENT_SELECT_H_
#define _IO_EVENT_SELECT_H_

#include "base/func.h"
#include "base/osdef.h"
#include "net/ioevent.h"
#include "base/lock.h"
#include "net/socket_op.h"

#include <map>
#include <vector>
#include <list>

namespace ff {

class IOEventSelect: public IOEvent
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
    IOEventSelect();
    virtual ~IOEventSelect();

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
    Socketfd                     m_fdNotify[2];
    struct  sockaddr_in             m_serAddr;
    std::vector<FuncToRun>          m_listFuncToRun;
};

}
#endif

#ifndef _IO_EVENT_H_
#define _IO_EVENT_H_

#include "base/func.h"
#include "base/osdef.h"

namespace ff {

enum IOEVENT_TYPE
{
    IOEVENT_ACCEPT     = 0x1,
    IOEVENT_RECV       = 0x2,
    IOEVENT_BROKEN     = 0x4,
    IOEVENT_BUILD_PKG  = 0x8//!socket before send,data may be needed to be build a header pkg 
};
class IOEvent
{
public:
    typedef Function<void(Socketfd, int, const char*, size_t)> IOEventFunc;
    typedef Function<void()> FuncToRun;
public:
    virtual ~IOEvent(){}

    virtual int run() = 0;
    virtual int runOnce(int ms) = 0;
    virtual int stop()     = 0;
    virtual int regfd(Socketfd fd, IOEventFunc eventHandler)          = 0;
    virtual int regfdAccept(Socketfd fd, IOEventFunc eventHandler)    = 0;
    virtual int unregfd(Socketfd fd)    = 0;

    virtual void asyncSend(Socketfd fd, const char* data, size_t len) = 0;
    virtual void post(FuncToRun func) = 0;
};

}
#endif

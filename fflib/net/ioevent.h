#ifndef _IO_EVENT_H_
#define _IO_EVENT_H_

#include "base/func.h"
#include "base/osdef.h"

namespace ff {

enum IOEVENT_TYPE
{
    IOEVENT_ACCEPT = 0x1,
    IOEVENT_RECV   = 0x2,
    IOEVENT_BROKEN = 0x4,
};
class IOEvent
{
public:
    typedef Function<void(SOCKET_TYPE, int, const char*, size_t)> IOEventFunc;
    typedef Function<void()> FuncToRun;
public:
    virtual ~IOEvent(){}

    virtual int run() = 0;
    virtual int runOnce(int ms) = 0;
    virtual int stop()     = 0;
    virtual int regfd(SOCKET_TYPE fd, IOEventFunc eventHandler)          = 0;
    virtual int regfdAccept(SOCKET_TYPE fd, IOEventFunc eventHandler)    = 0;
    virtual int unregfd(SOCKET_TYPE fd)    = 0;

    virtual void asyncSend(SOCKET_TYPE fd, const char* data, size_t len) = 0;
    virtual void post(FuncToRun func) = 0;
};

}
#endif

#ifndef _MSG_HANDLER_XX_H_
#define _MSG_HANDLER_XX_H_

#include "net/socket.h"
#include "net/message.h"

namespace ff {
class TaskQueue;

class MsgHandler
{
public:
    virtual ~MsgHandler() {} ;

    virtual int handleBroken(SocketObjPtr sock_)  = 0;
    virtual int handleMsg(const Message& msg_, SocketObjPtr sock_) = 0;

    virtual TaskQueue* getTaskQueue() = 0;
};

typedef MsgHandler* MsgHandlerPtr;
}

#endif

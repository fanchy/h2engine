#ifndef _MSG_HANDLER_XX_H_
#define _MSG_HANDLER_XX_H_

#include "net/socket.h"
#include "net/message.h"

namespace ff {
class TaskQueueI;

class MsgHandlerI
{
public:
    virtual ~MsgHandlerI() {} ;

    virtual int handleBroken(socket_ptr_t sock_)  = 0;
    virtual int handleMsg(const Message& msg_, socket_ptr_t sock_) = 0;

    virtual TaskQueueI* getTaskQueue() = 0;
};

typedef MsgHandlerI* msg_handler_ptr_t;
}

#endif

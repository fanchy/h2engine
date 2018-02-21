#ifndef _COMMON_SOCKET_CONTROLLER_H_
#define _COMMON_SOCKET_CONTROLLER_H_

#include <string>


#include "net/socket_ctrl.h"
#include "net/message.h"
#include "net/msg_handler.h"

namespace ff {

class SocketI;

class SocketCtrlCommon: public SocketCtrlI
{
public:
    SocketCtrlCommon(MsgHandlerPtr msg_handler_);
    ~SocketCtrlCommon();
    virtual int handleError(SocketI*);
    virtual int handleRead(SocketI*, const char* buff, size_t len);
    virtual int handleWriteCompleted(SocketI*);

    virtual int checkPreSend(SocketI*, const std::string& buff, int flag);

    virtual void post_msg(SocketI* sp_);
protected:
    MsgHandlerPtr   m_msg_handler;
    size_t              m_have_recv_size;
    Message           m_message;
};

}

#endif

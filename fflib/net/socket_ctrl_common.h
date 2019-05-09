#ifndef _COMMON_SOCKET_CONTROLLER_H_
#define _COMMON_SOCKET_CONTROLLER_H_

#include <string>


#include "net/socket_ctrl.h"
#include "net/message.h"
#include "net/msg_handler.h"
#include "net/wsprotocol.h"

namespace ff {

class SocketI;

class SocketCtrlCommon: public SocketCtrlI
{
public:
    SocketCtrlCommon(MsgHandlerPtr msg_handler_);
    ~SocketCtrlCommon();
    virtual int handleOpen(SocketI*);
    virtual int handleError(SocketI*);
    virtual int handleRead(SocketI*, const char* buff, size_t len);
    virtual int handleWriteCompleted(SocketI*);

    virtual int checkPreSend(SocketI*, std::string& buff, int flag);

    virtual void post_msg(SocketI* sp_);
    virtual int get_type() const;
protected:
    MsgHandlerPtr       m_msg_handler;
    size_t              m_have_recv_size;
    Message             m_message;
    WSProtocol          m_oWSProtocol;
};

}

#endif

#ifndef _ACCEPTOR_TCP_
#define _ACCEPTOR_TCP_

#include <string>
#include "base/osdef.h"
#include "net/ioevent.h"
#include "net/acceptor.h"

namespace ff {

#define LISTEN_BACKLOG 5

class IOEvent;
class SocketI;
class MsgHandler;
class TaskQueue;

class AcceptorTcp:public Acceptor
{
public:
    AcceptorTcp(IOEvent&, MsgHandler*, TaskQueue* tq_);
    ~AcceptorTcp();
    int   open(const std::string& address_);
    void  close();

    virtual SOCKET_TYPE socket(){ return m_fdListen; }
protected:
    void handleEvent(SOCKET_TYPE fdEvent, int eventType, const char* data, size_t len);
protected:
    int                 m_fdListen;
    IOEvent&            m_ioevent;
    MsgHandler*         m_msgHandler;
    TaskQueue*          m_tq;
};
}
#endif

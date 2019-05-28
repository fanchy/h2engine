#ifndef _ACCEPTOR_TCP_
#define _ACCEPTOR_TCP_

#include <string>
#include "base/osdef.h"
#include "net/ioevent.h"
#include "net/acceptor.h"
#include "net/socket_protocol.h"


namespace ff {

#define LISTEN_BACKLOG 5

class IOEvent;
class SocketObj;
class TaskQueue;

class AcceptorTcp:public Acceptor
{
public:
    AcceptorTcp(IOEvent&, SocketProtocolFunc);
    ~AcceptorTcp();
    int   open(const std::string& address_);
    void  close();

    virtual Socketfd getRawSocket(){ return m_fdListen; }
protected:
    void handleEvent(Socketfd fdEvent, int eventType, const char* data, size_t len);
protected:
    int                 m_fdListen;
    IOEvent&            m_ioevent;
    SocketProtocolFunc  m_funcProtocol;
};
}
#endif

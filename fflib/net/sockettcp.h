#ifndef _SOCKET_TCP_H_
#define _SOCKET_TCP_H_

#include <list>
#include <string>

#include "net/socket.h"
#include "net/socket_op.h"
#include "net/ioevent.h"
#include "base/func.h"

namespace ff {

class IOEvent;
class SocketCtrlI;
class TaskQueue;

#define  RECV_BUFFER_SIZE 8192

class SocketTcp: public SocketObj
{
public:
    SocketTcp(IOEvent&, SocketEventFunc, Socketfd fd);
    ~SocketTcp();

    virtual Socketfd getRawSocket() { return m_fd; }
    virtual void close();
    virtual void open();

    virtual void sendRaw(const std::string& buff_);
    
    void refSelf(SharedPtr<SocketTcp> p);
private:
    void handleEvent(Socketfd fdEvent, int eventType, const char* buff, size_t len);

private:
    IOEvent&                         m_ioevent;
    SocketEventFunc                  m_funcSocketEvent;
    Socketfd                         m_fd;
    SharedPtr<SocketTcp>             m_refSocket;//control socket life

};

}

#endif

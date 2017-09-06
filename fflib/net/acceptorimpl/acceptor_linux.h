#ifndef _ACCEPTOR_IMPL_H_
#define _ACCEPTOR_IMPL_H_

#include "net/acceptor.h"

namespace ff {

#define LISTEN_BACKLOG 5

class EventLoopI;
class SocketI;
class MsgHandlerI;
class TaskQueuePool;

class AcceptorLinux: public acceptor_i
{
public:
    AcceptorLinux(EventLoopI*, MsgHandlerI*, TaskQueuePool* tq_);
    ~AcceptorLinux();
    int   open(const std::string& address_);
    void close();

    socket_fd_t socket() {return m_listen_fd;}
    int handleEpollRead();
    int handleEpollDel() ;
    
protected:
    virtual SocketI* create_socket(socket_fd_t);

protected:
    int                 m_listen_fd;
    EventLoopI*       m_epoll;
    MsgHandlerI*      m_msg_handler;
    TaskQueuePool*  m_tq;
};

}

#endif


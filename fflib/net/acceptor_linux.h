#ifndef _ACCEPTOR_IMPL_H_
#define _ACCEPTOR_IMPL_H_

#include "net/acceptor.h"

namespace ff {

#define LISTEN_BACKLOG 5

class EventLoop;
class SocketI;
class MsgHandler;
class TaskQueue;

class AcceptorLinux: public Acceptor
{
public:
    AcceptorLinux(EventLoop*, MsgHandler*, TaskQueue* tq_);
    ~AcceptorLinux();
    int   open(const std::string& address_);
    void close();

    SocketFd socket() {return m_listen_fd;}
    int handleEpollRead();
    int handleEpollDel() ;

protected:
    virtual SocketI* create_socket(SocketFd);

protected:
    int                 m_listen_fd;
    EventLoop*          m_epoll;
    MsgHandler*         m_msg_handler;
    TaskQueue*          m_tq;
};

}

#endif

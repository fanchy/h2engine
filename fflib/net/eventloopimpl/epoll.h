#ifndef _EPOLL_IMPL_H_
#define _EPOLL_IMPL_H_

#include <list>


#include "net/event_loop.h"
#include "base/task_queue.h"
#include "base/lock.h"

namespace ff {

#define CREATE_EPOLL_SIZE  4096
#define EPOLL_EVENTS_SIZE  128
//! 100 ms
#define EPOLL_WAIT_TIME    -1

class Epoll: public EventLoop
{
public:
    Epoll();
    ~Epoll();

    virtual int runLoop();
    virtual int close();
    virtual int registerfd(Fd*);
    virtual int unregisterfd(Fd*);
    virtual int modfd(Fd*);

    int interupt_loop();//! 中断事件循环
protected:
    void fd_del_callback();

private:
    volatile bool            m_running;
    int                      m_efd;
    TaskQueue*               m_task_queue;
    int                      m_interupt_sockets[2];
    //! 待销毁的error socket
    std::list<Fd*>           m_error_fd_set;
    Mutex                    m_mutex;
};

}
#endif

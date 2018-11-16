#ifndef _WIN32
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "net/socket_op.h"
#include "net/netbase.h"
#include "net/eventloopimpl/epoll.h"
using namespace std;
using namespace ff;

Epoll::Epoll():
    m_running(true),
    m_efd(-1)
{
    m_efd = ::epoll_create(CREATE_EPOLL_SIZE);
    m_interupt_sockets[0] = -1;
    m_interupt_sockets[1] = -1;
    assert( 0 == ::socketpair(AF_LOCAL, SOCK_STREAM, 0, m_interupt_sockets));
    struct epoll_event ee = { 0, { 0 } };
	ee.data.ptr  = this;
	ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;
	::epoll_ctl(m_efd, EPOLL_CTL_ADD, m_interupt_sockets[0], &ee);
}

Epoll::~Epoll()
{
    ::close(m_interupt_sockets[0]);
    ::close(m_interupt_sockets[1]);
    ::close(m_efd);
    m_efd = -1;
}

int Epoll::runLoop()
{
    int i = 0, nfds = 0;
    struct epoll_event ev_set[EPOLL_EVENTS_SIZE];

    do
    {
        nfds  = ::epoll_wait(m_efd, ev_set, EPOLL_EVENTS_SIZE, EPOLL_WAIT_TIME);

        if (nfds < 0 && EINTR == errno)
        {
            nfds = 0;
            continue;
        }
        for (i = 0; i < nfds; ++i)
        {
            epoll_event& cur_ev = ev_set[i];
            Fd* fd_ptr	    = (Fd*)cur_ev.data.ptr;
            if (cur_ev.data.ptr == this)//! iterupte event
            {
            	if (false == m_running)
				{
            		return 0;
				}

            	//! 删除那些已经出现error的socket 对象
            	fd_del_callback();
            	continue;
            }
    
            if (cur_ev.events & (EPOLLIN | EPOLLPRI))
            {
                fd_ptr->handleEpollRead();
            }

            if(cur_ev.events & EPOLLOUT)
            {
                fd_ptr->handleEpollWrite();
            }

            if (cur_ev.events & (EPOLLERR | EPOLLHUP))
            {
                fd_ptr->close();
            }
        }
        
    }while(nfds >= 0);

    return 0;
}

int Epoll::close()
{
    m_running = false;

    interupt_loop();
    
    return 0;
}

int Epoll::register_fd(Fd* fd_ptr_)
{
    struct epoll_event ee = { 0, { 0 } };

    ee.data.ptr  = fd_ptr_;
    ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;

    return ::epoll_ctl(m_efd, EPOLL_CTL_ADD, fd_ptr_->socket(), &ee);
}

int Epoll::unregister_fd(Fd* fd_ptr_)
{
	int ret = 0;
	if (fd_ptr_->socket() > 0)
	{
		struct epoll_event ee;

		ee.data.ptr  = (void*)0;
		ret = ::epoll_ctl(m_efd, EPOLL_CTL_DEL, fd_ptr_->socket(), &ee);
	}

	{
		LockGuard lock(m_mutex);
		m_error_fd_set.push_back(fd_ptr_);
	}
    interupt_loop();
    return ret;
}

int Epoll::mod_fd(Fd* fd_ptr_)
{
    struct epoll_event ee = { 0, { 0 } };

    ee.data.ptr  = fd_ptr_;
    ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;

    return ::epoll_ctl(m_efd, EPOLL_CTL_MOD, fd_ptr_->socket(), &ee);
}

void Epoll::fd_del_callback()
{
    LockGuard lock(m_mutex);
    list<Fd*>::iterator it = m_error_fd_set.begin();
    for (; it != m_error_fd_set.end(); ++it)
    {
        (*it)->handleEpollDel();
    }
    m_error_fd_set.clear();
}

int Epoll::interupt_loop()//! 中断事件循环
{
	struct epoll_event ee = { 0, { 0 } };

	ee.data.ptr  = this;
	ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;

	return ::epoll_ctl(m_efd, EPOLL_CTL_MOD, m_interupt_sockets[0], &ee);
}

#endif


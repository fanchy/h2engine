#ifdef _WIN32
#ifndef _WIN32
#include <sys/epoll.h>
#include <sys/socket.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <assert.h>

#include "net/socket_op.h"
#include "net/netbase.h"
#include "net/eventloopimpl/select.h"

using namespace ff;

Select::Select():
    m_running(true),
    m_efd(-1)
{
    /*TODO m_efd = ::epoll_create(CREATE_EPOLL_SIZE);
    m_interupt_sockets[0] = -1;
    m_interupt_sockets[1] = -1;
    assert( 0 == ::socketpair(AF_LOCAL, SOCK_STREAM, 0, m_interupt_sockets));
    struct epoll_event ee = { 0, { 0 } };
	ee.data.ptr  = this;
	ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;
	::epoll_ctl(m_efd, EPOLL_CTL_ADD, m_interupt_sockets[0], &ee);
	*/
	
}

Select::~Select()
{
    ::close(m_interupt_sockets[0]);
    ::close(m_interupt_sockets[1]);
    ::close(m_efd);
    m_efd = -1;
}

int Select::runLoop()
{
	FD_ZERO(&m_fdread);
	struct timeval tv = {0, 0};
	tv.tv_sec  = 0;  
	tv.tv_usec = 1000*200;  
	SocketFd tmpsock = socket(AF_INET, SOCK_STREAM, 0);
	while (m_running)
	{     
		// We only care read event 
		fd_set tmpset;
		FD_ZERO(&tmpset);
		{
			LockGuard lock(m_mutex);
			tmpset = m_fdread;
		}
		
		FD_SET(tmpsock, &tmpset);
		int ret = ::select(0, &tmpset, NULL, NULL, &tv);
		if (ret == 0) 
		{
			// Time expired 
		 	continue; 
		}
		else if (ret < 0){
			fprintf(stderr, "Select::runLoop: %d\n", WSAGetLastError());
			continue;
		}
		fd_del_callback();
		for (int i = 0; i < tmpset.fd_count; i++ )
		{
			if (FD_ISSET(tmpset.fd_array[i], &tmpset))
			{
			   map<SocketFd, Fd*>::iterator it = m_fd2ptr.find(tmpset.fd_array[i]);
			   if (it!= m_fd2ptr.end()){
			   		Fd* fd_ptr = it->second;
					fd_ptr->handleEpollRead();
			   }
			   
		   } //if 
		}//for 
	}//while  
	SocketOp::close(tmpsock);
	/*
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
	*/
    return 0;
}

int Select::close()
{
    m_running = false;

    interupt_loop();
    
    return 0;
}

int Select::register_fd(Fd* fd_ptr_)
{
	LockGuard lock(m_mutex);
    FD_SET(fd_ptr_->socket(), &m_fdread);
    m_fd2ptr[fd_ptr_->socket()] = fd_ptr_;
    //printf("elect_t::register_fd %d\n", fd_ptr_->socket());
    return 0;
}

int Select::unregister_fd(Fd* fd_ptr_)
{
	LockGuard lock(m_mutex);
	if  (m_fd2ptr.find(fd_ptr_->socket()) == m_fd2ptr.end())
		return 0;
	int ret = 0;
	FD_CLR(fd_ptr_->socket(), &m_fdread);
	m_fd2ptr.erase(fd_ptr_->socket());
	//printf("elect_t::unregister_fd %d\n", fd_ptr_->socket());
	m_error_fd_set.push_back(fd_ptr_);
	interupt_loop();
    return ret;
}

int Select::mod_fd(Fd* fd_ptr_)
{
    return 0;
}

void Select::fd_del_callback()
{
    LockGuard lock(m_mutex);
    list<Fd*>::iterator it = m_error_fd_set.begin();
    for (; it != m_error_fd_set.end(); ++it)
    {
        (*it)->handleEpollDel();
    }
    m_error_fd_set.clear();
}

int Select::interupt_loop()//! 中断事件循环
{
	/*
	struct epoll_event ee = { 0, { 0 } };

	ee.data.ptr  = this;
	ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;

	return ::epoll_ctl(m_efd, EPOLL_CTL_MOD, m_interupt_sockets[0], &ee);
	*/
	return 0;
}
#endif


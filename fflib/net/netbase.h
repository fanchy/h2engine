#ifndef _NET_BASE_H_
#define _NET_BASE_H_

#include "base/fftype.h"

namespace ff {

//! 文件描述符相关接口
typedef SOCKET_TYPE socket_fd_t;
class Fd
{
public:
    virtual ~Fd(){}

    virtual socket_fd_t socket() 	 = 0;
    virtual int handleEpollRead()  = 0;
    virtual int handleEpollWrite() = 0;
    virtual int handleEpollDel() 	 = 0;

    virtual void close() 			 = 0;
};


//typedef Fd epoll_Fd;
}

#endif



#ifndef _NET_BASE_H_
#define _NET_BASE_H_

#include "base/fftype.h"

namespace ff {

//! 文件描述符相关接口
typedef SOCKET_TYPE SocketFd;
class Fd
{
public:
    virtual ~Fd(){}

    virtual SocketFd socket() 	 = 0;
    virtual int handleEpollRead()  = 0;
    virtual int handleEpollWrite() = 0;
    virtual int handleEpollDel() 	 = 0;

    virtual void close() 			 = 0;
};


//typedef Fd epoll_Fd;
}

#endif



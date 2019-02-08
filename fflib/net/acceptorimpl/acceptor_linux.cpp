#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#endif


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <vector>
using namespace std;

#include "net/event_loop.h"
#include "net/acceptorimpl/acceptor_linux.h"
#include "net/socketimpl/socket_linux.h"
#include "net/socketimpl/socket_win.h"
#include "net/ctrlimpl/socket_ctrl_common.h"
#include "base/str_tool.h"
#include "net/socket_op.h"
#include "net/msg_handler.h"
#include "base/task_queue.h"
#include "base/log.h"

using namespace ff;

AcceptorLinux::AcceptorLinux(EventLoop* e_, MsgHandler* msg_handler_, TaskQueuePool* tq_):
    m_listen_fd(-1),
    m_epoll(e_),
    m_msg_handler(msg_handler_),
    m_tq(tq_)
{
}

AcceptorLinux::~AcceptorLinux()
{
    this->close();
}

int AcceptorLinux::open(const string& address_)
{
    //! example tcp://192.168.1.2:8080
    vector<string> vt;
    StrTool::split(address_, vt, "://");
    if(vt.size() != 2) return -1;

    vector<string> vt2;
    StrTool::split(vt[1], vt2, ":");
    if(vt2.size() != 2) return -1;

    struct addrinfo hints;
    #ifdef _WIN32
    ZeroMemory(&hints, sizeof(struct addrinfo) );
    #else
    bzero(&hints, sizeof(struct addrinfo) );
    #endif
    hints.ai_family      = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;
    hints.ai_flags        = AI_PASSIVE;

    int ret =0, yes = 1;
    struct addrinfo *res;

    const char* host = NULL;
    if (vt2[0] != "*") host = vt2[0].c_str();
    #ifdef _WIN32
    if (vt2[0] == "*")
    	host = "127.0.0.1";
    #endif

    if ((ret = getaddrinfo(host, vt2[1].c_str(), &hints, &res)) != 0)
    {
        fprintf(stderr, "AcceptorLinux::open getaddrinfo: %s, address_=<%s>\n", gai_strerror(ret), address_.c_str());
        return -1;
    }

    SocketFd tmpfd = -1;
    SocketFd nInvalid = -1;
    #ifdef _WIN32
        nInvalid = INVALID_SOCKET;
    #endif
    if ((tmpfd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == nInvalid)
    {
        perror("AcceptorLinux::open when socket");
        return -1;
    }

    if (::setsockopt(tmpfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int)) == -1)
    {
        perror("AcceptorLinux::open when setsockopt");
        return -1;
    }
    if (::bind(tmpfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "AcceptorLinux::open when bind: %s, address_=<%s>\n", strerror(errno), address_.c_str());
        return -1;
    }

    SocketOp::set_nonblock(tmpfd);
    if (::listen(tmpfd, LISTEN_BACKLOG) == -1)
    {
        perror("AcceptorLinux::open when listen");
        return -1;
    }
    ::freeaddrinfo(res);
    m_listen_fd = tmpfd;
    return m_epoll->register_fd(this);
}

void AcceptorLinux::close()
{
    if (m_listen_fd > 0)
    {
        m_epoll->unregister_fd(this);
        SocketOp::close(m_listen_fd);
        m_listen_fd = -1;
    }
}

int AcceptorLinux::handleEpollRead()
{

    SocketFd new_fd = -1;
    do
    {
    	#ifdef _WIN32
        	new_fd = accept(m_listen_fd, NULL, NULL);
    	    if (new_fd == INVALID_SOCKET) {
    	        wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
    	        return 1;
    	    }
    	    SocketOp::set_no_delay(new_fd);
            SocketObjPtr socket = create_socket(new_fd);
            socket->refSelf(socket);
            socket->open();
            return 0;
    	#else
            struct sockaddr_storage addr;
            socklen_t addrlen = sizeof(addr);
            if ((new_fd = ::accept(m_listen_fd, (struct sockaddr *)&addr, &addrlen)) == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    return 0;
                }
                else if (errno == EINTR || errno == EMFILE || errno == ECONNABORTED || errno == ENFILE ||
                            errno == EPERM || errno == ENOBUFS || errno == ENOMEM)
                {
                    perror("accept");//! if too many open files occur, need to restart epoll event
                    m_epoll->mod_fd(this);
                    return 0;
                }
                perror("accept other error");
                return -1;
            }

            SocketOp::set_no_delay(new_fd);
            SocketObjPtr socket = create_socket(new_fd);
            socket->refSelf(socket);
            LOGINFO(("FFNET", "SocketLinux::SocketLinux  %x", (long)(SMART_PTR_RAW(socket))));
            socket->open();
        #endif
    } while (true);
    return 0;
}

int AcceptorLinux::handleEpollDel()
{
    return 0;
}

SocketI* AcceptorLinux::create_socket(SocketFd new_fd_)
{
	#ifdef _WIN32
    return new SocketWin(m_epoll, new SocketCtrlCommon(m_msg_handler), new_fd_, m_tq->alloc(new_fd_));
	#else
    return new SocketLinux(m_epoll, new SocketCtrlCommon(m_msg_handler), new_fd_, m_tq->alloc(new_fd_));
	#endif
}

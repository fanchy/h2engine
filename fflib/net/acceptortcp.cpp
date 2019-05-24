
#include "base/osdef.h"
#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#define _WIN32_WINNT 0x0501
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

#include "net/ioevent.h"
#include "net/acceptortcp.h"
#include "net/sockettcp.h"
#include "base/str_tool.h"
#include "net/socket_op.h"
#include "net/msg_handler.h"
#include "base/task_queue.h"
#include "base/log.h"
#include "net/socket_ctrl_common.h"

using namespace ff;

AcceptorTcp::AcceptorTcp(IOEvent& e_, MsgHandler* msg_handler_, TaskQueue* tq_):
    m_fdListen(-1),
    m_ioevent(e_),
    m_msgHandler(msg_handler_),
    m_tq(tq_)
{
}

AcceptorTcp::~AcceptorTcp()
{
    this->close();
}

int AcceptorTcp::open(const string& address_)
{
    LOGTRACE(("ACCEPTOR", "accept begin %s", address_));
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
        fprintf(stderr, "AcceptorTcp::open getaddrinfo: %s, address_=<%s>\n", gai_strerror(ret), address_.c_str());
        return -1;
    }

    SOCKET_TYPE tmpfd = -1;
    SOCKET_TYPE nInvalid = -1;
    #ifdef _WIN32
        nInvalid = INVALID_SOCKET;
    #endif
    if ((tmpfd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == nInvalid)
    {
        perror("AcceptorTcp::open when socket");
        return -1;
    }

    if (::setsockopt(tmpfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int)) == -1)
    {
        perror("AcceptorTcp::open when setsockopt");
        return -1;
    }
    if (::bind(tmpfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        fprintf(stderr, "AcceptorTcp::open when bind: %s, address_=<%s>\n", strerror(errno), address_.c_str());
        return -1;
    }

    SocketOp::set_nonblock(tmpfd);
    if (::listen(tmpfd, LISTEN_BACKLOG) == -1)
    {
        perror("AcceptorTcp::open when listen");
        return -1;
    }
    ::freeaddrinfo(res);
    m_fdListen = tmpfd;
    LOGTRACE(("ACCEPTOR", "accept ok %s", address_));
    return m_ioevent.regfdAccept(m_fdListen, funcbind(&AcceptorTcp::handleEvent, this));
}
void AcceptorTcp::handleEvent(SOCKET_TYPE fdEvent, int eventType, const char* data, size_t len)
{
    LOGTRACE(("ACCEPTOR", "AcceptorTcp::handleEvent eventType=%d", eventType));
    if (eventType != IOEVENT_ACCEPT){
        return;
    }
    SOCKET_TYPE newFd = *((SOCKET_TYPE*)data);
    SocketObjPtr socket = new SocketTcp(m_ioevent, new SocketCtrlCommon(m_msgHandler), newFd, m_tq);
    socket->refSelf(socket);
    socket->open();
}
void AcceptorTcp::close()
{
    if (m_fdListen > 0)
    {
        m_ioevent.unregfd(m_fdListen);
        SocketOp::close(m_fdListen);
        m_fdListen = -1;
    }
}

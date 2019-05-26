
#include "base/osdef.h"

#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif // _WIN32

#include "net/sockettcp.h"
#include "net/socket_op.h"
#include "base/task_queue.h"
#include "base/log.h"
#define FFNET "FFNET"

using namespace std;
using namespace ff;

SocketTcp::SocketTcp(IOEvent& e_, SocketEventFunc func, Socketfd fd_):
    m_ioevent(e_),
    m_funcSocketEvent(func),
    m_fd(fd_)
{
}

SocketTcp::~SocketTcp()
{
    LOGTRACE((FFNET, "SocketTcp::~SocketTcp  %x", (long)this));
}

void SocketTcp::open()
{
    m_ioevent.regfd(m_fd, funcbind(&SocketTcp::handleEvent, this));
}
void SocketTcp::handleEvent(Socketfd fdEvent, int eventType, const char* data, size_t len)
{
    LOGTRACE((FFNET, "SocketTcp::handleEvent event=%s len=%u", eventType == IOEVENT_RECV? "IOEVENT_RECV": "IOEVENT_BROKEN", (unsigned int)len));
    switch (eventType){
        case IOEVENT_RECV:
        {
            m_funcSocketEvent(m_refSocket, eventType, data, len);
        }break;
        case IOEVENT_BROKEN:
        {
            this->close();
        }break;
        default:break;
    }
}
void SocketTcp::close()
{
    LOGTRACE((FFNET, "SocketTcp::close %p", (long)this));
    if (m_fd > 0)
    {
        Socketfd fd = m_fd;
        m_fd = -1;
        m_ioevent.unregfd(fd);
        m_funcSocketEvent(m_refSocket, IOEVENT_BROKEN, "", 0);
    }
    if (m_refSocket){
        LOGTRACE((FFNET, "SocketTcp::close  %x", (long)this));
        m_refSocket = NULL;
    }
}

void SocketTcp::sendRaw(const string& msg_)
{
    m_ioevent.asyncSend(m_fd, msg_.c_str(), msg_.size());
}

SharedPtr<SocketObj> SocketTcp::toSharedPtr(){
    return m_refSocket;
}

void SocketTcp::refSelf(SharedPtr<SocketObj> p){
    m_refSocket = p;
}

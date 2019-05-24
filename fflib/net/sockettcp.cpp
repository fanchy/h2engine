
#include "base/osdef.h"

#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "net/sockettcp.h"
#include "net/socket_ctrl.h"
#include "net/socket_op.h"
#include "base/task_queue.h"
#include "base/log.h"
#define FFNET "FFNET"

using namespace std;
using namespace ff;

SocketTcp::SocketTcp(IOEvent& e_, SocketCtrlI* seh_, int fd_, TaskQueue* tq_):
    m_ioevent(e_),
    m_sc(seh_),
    m_fd(fd_),
    m_tq(tq_)
{
}

SocketTcp::~SocketTcp()
{
    delete m_sc;
    LOGTRACE((FFNET, "SocketTcp::~SocketTcp  %x", (long)this));
}

void SocketTcp::open()
{
    m_ioevent.regfd(m_fd, funcbind(&SocketTcp::handleEvent, this));
}
void SocketTcp::handleEvent(SOCKET_TYPE fdEvent, int eventType, const char* data, size_t len)
{
    LOGTRACE((FFNET, "SocketTcp::handleEvent event=%s len=%u", eventType == IOEVENT_RECV? "IOEVENT_RECV": "IOEVENT_BROKEN", (unsigned int)len));
    switch (eventType){
        case IOEVENT_RECV:
        {
            m_sc->handleRead(this, data, len);
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
        m_ioevent.unregfd(m_fd);
        m_fd = -1;
        this->getSocketCtrl()->handleError(this);
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
void SocketTcp::asyncSend(const string& msg_)
{
    string buff_ = msg_;
    m_sc->checkPreSend(this, buff_, 0);
    m_ioevent.asyncSend(m_fd, buff_.c_str(), buff_.size());
}

SharedPtr<SocketI> SocketTcp::toSharedPtr(){
    return m_refSocket;
}

void SocketTcp::refSelf(SharedPtr<SocketI> p){
    m_refSocket = p;
}

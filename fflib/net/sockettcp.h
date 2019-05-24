#ifndef _SOCKET_TCP_H_
#define _SOCKET_TCP_H_

#include <list>
#include <string>


#include "net/socket.h"
#include "base/obj_pool.h"
#include "net/socket_op.h"
#include "net/ioevent.h"

namespace ff {

class IOEvent;
class SocketCtrlI;
class TaskQueue;

#define  RECV_BUFFER_SIZE 8192

class SocketTcp: public SocketI
{
public:
    SocketTcp(IOEvent&, SocketCtrlI*, SOCKET_TYPE fd, TaskQueue* tq_);
    ~SocketTcp();

    virtual SOCKET_TYPE socket() { return m_fd; }
    virtual void close();
    virtual void open();

    virtual void sendRaw(const std::string& buff_);
    virtual void asyncSend(const std::string& buff_);
    
    SocketCtrlI* getSocketCtrl() { return m_sc; }
    virtual SharedPtr<SocketI> toSharedPtr();
    void refSelf(SharedPtr<SocketI> p);
private:
    void handleEvent(SOCKET_TYPE fdEvent, int eventType, const char* data, size_t len);
    bool isOpen() { return m_fd > 0; }

private:
    IOEvent&                         m_ioevent;
    SocketCtrlI*                     m_sc;
    SOCKET_TYPE                         m_fd;
    TaskQueue*                       m_tq;
    SocketObjPtr                     m_refSocket;//control socket life
};

}

#endif

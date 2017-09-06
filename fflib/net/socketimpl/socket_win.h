#ifndef _SOCKET_IMPL_WIN_H_
#define _SOCKET_IMPL_WIN_H_
#ifdef _WIN32
#include <list>
#include <string>


#include "net/socket.h"
#include "base/obj_pool.h"

namespace ff {

class EventLoopI;
class SocketCtrlI;
class TaskQueueI;

#define  RECV_BUFFER_SIZE 8096


class SocketWin: public SocketI
{
public:
    struct ff_str_buffer_t
    {
        ff_str_buffer_t():has_send_size(0),flag(0){}
        int    has_send_size;
        int    flag;
        std::string buff;
        
        int         left_size() const { return buff.size() - has_send_size;  }
        const char* cur_data()  const { return buff.c_str() + has_send_size; }
        void        consume(int n)    { has_send_size += n;                  }
    };
    typedef std::list<ff_str_buffer_t>    send_buffer_t;

public:
    SocketWin(EventLoopI*, SocketCtrlI*, socket_fd_t fd, TaskQueueI* tq_);
    ~SocketWin();

    virtual socket_fd_t socket() { return m_fd; }
    virtual void close();
    virtual void open();

    virtual int handleEpollRead();
    virtual int handleEpollWrite();
    virtual int handleEpollDel();

    virtual void asyncSend(const std::string& buff_);
    virtual void asyncRecv();
    virtual void safeDelete();
    
    int handleEpollRead_impl();
    int handleEpollWrite_impl();
    int handle_epoll_error_impl();

    virtual void sendRaw(const std::string& buff_);
    void send_str_impl(const std::string& buff_);
    void send_impl(const std::string& buff_, int flag = 0);
    void close_impl();
    
    SocketCtrlI* get_sc() { return m_sc; }
    
    void post_recv_msg(const std::string& data);
private:
    bool is_open() { return m_fd > 0; }

    int do_send(ff_str_buffer_t* buff_);
private:
    EventLoopI*                       m_epoll;
    SocketCtrlI*                      m_sc;
    socket_fd_t                         m_fd;
    TaskQueueI*                       m_tq;
    send_buffer_t                       m_send_buffer;
};

}

#endif
#endif


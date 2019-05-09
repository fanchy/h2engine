#ifndef _WIN32

#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "net/socket_linux.h"
#include "net/event_loop.h"
#include "net/socket_ctrl.h"
#include "net/socket_op.h"
#include "base/singleton.h"
#include "base/lock.h"
#include "base/task_queue.h"
#include "base/log.h"
#define FFNET "FFNET"

using namespace std;
using namespace ff;

SocketLinux::SocketLinux(EventLoop* e_, SocketCtrlI* seh_, int fd_, TaskQueue* tq_):
    m_epoll(e_),
    m_sc(seh_),
    m_fd(fd_),
    m_tq(tq_)
{
}

SocketLinux::~SocketLinux()
{
    m_sendBuffer.clear();
    delete m_sc;
    LOGTRACE((FFNET, "SocketLinux::~SocketLinux  %x", (long)this));
}

void SocketLinux::open()
{
    SocketOp::set_nonblock(m_fd);
    m_sc->handleOpen(this);
    asyncRecv();
}

void SocketLinux::close()
{
    m_tq->post(TaskBinder::gen(&SocketLinux::closeImpl, this));
}

void SocketLinux::closeImpl()
{
    if (m_fd > 0)
    {
        m_epoll->unregisterfd(this);
        ::close(m_fd);
        m_fd = -1;
    }
}

int SocketLinux::handleEpollRead()
{
    m_tq->post(TaskBinder::gen(&SocketLinux::handleEpollRead_impl, this));
    return 0;
}

int SocketLinux::handleEpollRead_impl()
{
    if (isOpen())
    {
        int nread = 0;
        char recv_buffer[RECV_BUFFER_SIZE];
        do
        {
            nread = ::read(m_fd, recv_buffer, sizeof(recv_buffer) - 1);
            if (nread > 0)
            {
                recv_buffer[nread] = '\0';
                if (m_refSocket)
                    m_sc->handleRead(this, recv_buffer, size_t(nread));
                else{
                    LOGERROR((FFNET, "SocketLinux::handleEpollRead_impl  exception %x", (long)this));
                }

                if (nread < int(sizeof(recv_buffer) - 1))
                {
                	break;//! equal EWOULDBLOCK
                }
            }
            else if (0 == nread) //! eof
            {
                this->close();
                return -1;
            }
            else
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else if (errno == EWOULDBLOCK)
                {
                    break;
                }
                else
                {
                    this->close();
                    return -1;
                }
            }
        } while(1);
    }
    return 0;
}
int SocketLinux::handleEpollErrorImpl(){
    if (m_refSocket){
        this->getSocketCtrl()->handleError(this);
        m_refSocket = NULL;
        LOGTRACE((FFNET, "SocketLinux::handleEpollErrorImpl  %x", (long)this));
    }
    else{
        LOGTRACE((FFNET, "SocketLinux::handleEpollErrorImpl error  %x", (long)this));
    }
    return 0;
}
int SocketLinux::handleEpollDel()
{
    m_tq->post(TaskBinder::gen(&SocketLinux::handleEpollErrorImpl, this));
    return 0;
}

int SocketLinux::handleEpollWrite()
{
    m_tq->post(TaskBinder::gen(&SocketLinux::handleEpollWrite_impl, this));
    return 0;
}

int SocketLinux::handleEpollWrite_impl()
{
    int ret = 0;

    if (false == isOpen() || true == m_sendBuffer.empty())
    {
        return 0;
    }

    do
    {
        ff_str_buffer_t& pbuff = m_sendBuffer.front();
        ret = doSend(&pbuff);

        if (ret < 0)
        {
            this ->close();
            return -1;
        }
        else if (ret > 0)
        {
            return 0;
        }
        else
        {
            m_sendBuffer.pop_front();
        }
    } while (false == m_sendBuffer.empty());
    if (m_refSocket)
        m_sc->handleWriteCompleted(this);
    return 0;
}

void SocketLinux::asyncSend(const string& msg_)
{
    m_tq->post(TaskBinder::gen(&SocketLinux::sendStrImpl, this, msg_));
}

void SocketLinux::sendStrImpl(const string& buff_)
{
    this->sendImpl(buff_, 1);
}
void SocketLinux::sendRaw(const string& buff_)
{
    this->sendImpl(buff_, 0);
}
void SocketLinux::sendImpl(const string& buff_src, int flag)
{
    string buff_ = buff_src;
    if (false == isOpen() || (flag != 0  && m_sc->checkPreSend(this, buff_, flag)))
    {
        return;
    }

    ff_str_buffer_t new_buff;
    new_buff.buff = buff_;
    new_buff.flag = flag;
    //! socket buff is full, cache the data
    if (false == m_sendBuffer.empty())
    {
        m_sendBuffer.push_back(new_buff);
        return;
    }

    int ret = doSend(&new_buff);

    if (ret < 0)
    {
        this ->close();
    }
    else if (ret > 0)//!left some data
    {
        m_sendBuffer.push_back(new_buff);
    }
    else
    {
        //! send ok
        m_sc->handleWriteCompleted(this);
    }
}

int SocketLinux::doSend(ff_str_buffer_t* buff_)
{
    size_t nleft             = (size_t)(buff_->left_size());
    const char* buffer       = buff_->cur_data();
    ssize_t nwritten;

    while(nleft > 0)
    {
        if((nwritten = ::send(m_fd, buffer, nleft, MSG_NOSIGNAL)) <= 0)
        {
            if (EINTR == errno)
            {
                nwritten = 0;
            }
            else if (EWOULDBLOCK == errno)
            {
                return 1;
            }
            else
            {
                this->close();
                return -1;
            }
        }

        nleft    -= nwritten;
        buffer   += nwritten;
        buff_->consume(nwritten);
    }

    return 0;
}

void SocketLinux::asyncRecv()
{
    m_epoll->registerfd(this);
}

SharedPtr<SocketI> SocketLinux::toSharedPtr(){
    return m_refSocket;
}

void SocketLinux::refSelf(SharedPtr<SocketI> p){
    m_refSocket = p;
}
#endif

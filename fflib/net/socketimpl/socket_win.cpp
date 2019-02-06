#ifdef _WIN32
#include <errno.h>
#include <assert.h>
#include <sys/types.h>

#ifndef WIN32
#include <sys/socket.h>
#endif

#include "net/socketimpl/socket_win.h"
#include "net/event_loop.h"
#include "net/socket_ctrl.h"
#include "net/socket_op.h"
#include "base/singleton.h"
#include "base/lock.h"
#include "base/task_queue.h"
#include "base/log.h"

using namespace ff;
using namespace std;

SocketWin::SocketWin(EventLoop* e_, SocketCtrlI* seh_, SocketFd fd_, TaskQueue* tq_):
    m_epoll(e_),
    m_sc(seh_),
    m_fd(fd_),
    m_tq(tq_)
{
}

SocketWin::~SocketWin()
{
    m_send_buffer.clear();
    delete m_sc;
}

void SocketWin::open()
{
    SocketOp::set_nonblock(m_fd);
    m_sc->handleOpen(this);
    asyncRecv();
}

void SocketWin::close()
{
    m_tq->post(TaskBinder::gen(&SocketWin::close_impl, this));
}

void SocketWin::close_impl()
{
    if (m_fd > 0)
    {
        m_epoll->unregister_fd(this);
        SocketOp::close(m_fd);
        m_fd = 0;
    }
}
void SocketWin::post_recv_msg(const string& data)
{
	m_sc->handleRead(this, data.c_str(), size_t(data.size()));
}
#ifdef _WIN32
int SocketWin::handleEpollRead()
{
    //printf("handleEpollRead_impl\n");
    if (is_open())
    {
        int nread = 0;
        char recv_buffer[RECV_BUFFER_SIZE];
    	nread = recv(m_fd, recv_buffer, RECV_BUFFER_SIZE, 0);
        if (nread == SOCKET_ERROR || nread == 0)
        {
        	if  (WSAEWOULDBLOCK == WSAGetLastError()){
        		return 0;
			}
        	//printf("xxxxxx recv err=%d\n",  WSAGetLastError());
            this->close();
            return -1;
        }
        else
        {
            recv_buffer[nread] = '\0';
            //printf("handleEpollRead_impl recv_buffer=%s\n", recv_buffer);
            string tmpdata;
            tmpdata.append(recv_buffer, nread);
            m_tq->post(TaskBinder::gen(&SocketWin::post_recv_msg, this, tmpdata));
        }
    }
    return 0;
}
#else
int SocketWin::handleEpollRead()
{
    m_tq->post(TaskBinder::gen(&SocketWin::handleEpollRead_impl, this));
    return 0;
}
#endif
int SocketWin::handleEpollRead_impl()
{
    if (is_open())
    {
        int nread = 0;
        char recv_buffer[RECV_BUFFER_SIZE];

        do
        {
            nread = ::read(m_fd, recv_buffer, sizeof(recv_buffer) - 1);
            if (nread > 0)
            {
                recv_buffer[nread] = '\0';
                m_sc->handleRead(this, recv_buffer, size_t(nread));
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

int SocketWin::handleEpollDel()
{
    m_tq->post(TaskBinder::gen(&SocketCtrlI::handleError, this->getSocketCtrl(), this));
    return 0;
}

int SocketWin::handleEpollWrite()
{
    m_tq->post(TaskBinder::gen(&SocketWin::handleEpollWrite_impl, this));
    return 0;
}

int SocketWin::handleEpollWrite_impl()
{
    int ret = 0;

    if (false == is_open() || true == m_send_buffer.empty())
    {
        return 0;
    }

    do
    {
        ff_str_buffer_t& pbuff = m_send_buffer.front();
        ret = do_send(&pbuff);

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
            m_send_buffer.pop_front();
        }
    } while (false == m_send_buffer.empty());

    m_sc->handleWriteCompleted(this);
    return 0;
}

void SocketWin::asyncSend(const string& msg_)
{
    m_tq->post(TaskBinder::gen(&SocketWin::send_str_impl, this, msg_));
}

void SocketWin::send_str_impl(const string& buff_)
{
    this->send_impl(buff_, 1);
}
void SocketWin::sendRaw(const string& buff_)
{
    this->send_impl(buff_, 0);
}
void SocketWin::send_impl(const string& buff_src, int flag)
{
    string buff_ = buff_src;
    if (false == is_open() || (flag != 0  && m_sc->checkPreSend(this, buff_, flag)))
    {
        return;
    }

    ff_str_buffer_t new_buff;
    new_buff.buff = buff_;
    new_buff.flag = flag;
    //! socket buff is full, cache the data
    if (false == m_send_buffer.empty())
    {
        m_send_buffer.push_back(new_buff);
        return;
    }

    int ret = do_send(&new_buff);

    if (ret < 0)
    {
        this ->close();
    }
    else if (ret > 0)//!left some data
    {
        m_send_buffer.push_back(new_buff);
    }
    else
    {
        //! send ok
        m_sc->handleWriteCompleted(this);
    }
}

int SocketWin::do_send(ff_str_buffer_t* buff_)
{
    size_t nleft             = (size_t)(buff_->left_size());
    const char* buffer       = buff_->cur_data();
    ssize_t nwritten;

    while(nleft > 0)
    {
    	#ifdef _WIN32
    	if((nwritten = ::send(m_fd, buffer, nleft, 0)) <= 0)
    	#else
        if((nwritten = ::send(m_fd, buffer, nleft, MSG_NOSIGNAL)) <= 0)
        #endif
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

void SocketWin::asyncRecv()
{
    m_epoll->register_fd(this);
}

void SocketWin::safeDelete()
{
    struct lambda_t
    {
        static void exe(void* p_)
        {
            delete ((SocketWin*)p_);
        }
    };
    m_tq->post(Task(&lambda_t::exe, this));
}
#endif

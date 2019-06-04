#ifdef linux

#include "base/func.h"
#include "base/osdef.h"
#include "net/ioevent_epoll.h"
#include "base/lock.h"
#include "net/socket_op.h"

#include <map>
#include <vector>
#include <list>
#include <sys/epoll.h>

using namespace std;
using namespace ff;

IOEventEpoll::IOEventEpoll():m_running(false){
    m_fdNotify[0] = 0;
    m_fdNotify[1] = 0;
    
    ::socketpair(AF_LOCAL, SOCK_STREAM, 0, m_fdNotify);
    
    m_efd = ::epoll_create1(0);
    
    struct epoll_event ee = { 0, { 0 } };
    ee.data.fd  = m_fdNotify[0];
	ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;
	::epoll_ctl(m_efd, EPOLL_CTL_ADD, m_fdNotify[0], &ee);
    //printf("m_fdNotify[0]=%d, m_fdNotify[1]=%d\n", m_fdNotify[0], m_fdNotify[1]);
}
IOEventEpoll::~IOEventEpoll(){
    stop();
    
    if (m_fdNotify[0]){
        close(m_fdNotify[0]);
        close(m_fdNotify[1]);
        m_fdNotify[0] = 0;
        m_fdNotify[1] = 0;
        ::close(m_efd);
        m_efd = -1;
    }
}

int IOEventEpoll::run()
{
    m_running = true;
    while (m_running)
    {
        runOnce(5000);
    }
    return 0;
}
int IOEventEpoll::runOnce(int ms)
{
    int nfds = ::epoll_wait(m_efd, ev_set, EPOLL_EVENTS_SIZE, ms);

    if (nfds < 0 && EINTR == errno)
    {
        nfds = 0;
        return nfds;
    }
    
    std::vector<IOCallBackInfo> listCallBack;
    std::vector<FuncToRun> listFunc;
    {
        LockGuard lock(m_mutex);
        listFunc = m_listFuncToRun;
        m_listFuncToRun.clear();
        //printf("epoll nfds=%d!\n", nfds);
        for (int i = 0; i < nfds; ++i)
        {
            epoll_event& cur_ev = ev_set[i];
            Socketfd fd = cur_ev.data.fd;

            if (cur_ev.data.fd == m_fdNotify[0])//! iterupte event
            {
                if (false == m_running)
                {
                    return 0;
                }
                //printf("epoll wake up!\n");
                continue;
            }
            //printf("epoll fd=%d!\n", fd);
            
            std::map<Socketfd, IOInfo>::iterator it = m_allIOinfo.find(fd);
            if (it == m_allIOinfo.end()){
                continue;
            }
            IOInfo& socketInfo = it->second;
            
            IOCallBackInfo cbInfo;
            cbInfo.fd = fd;
            cbInfo.eventHandler = socketInfo.eventHandler;
            if (cur_ev.events & (EPOLLIN | EPOLLPRI))
            {
                if (socketInfo.bAccept){
                    cbInfo.eventType = IOEVENT_ACCEPT;
                    int newfd = SocketOp::acceptSocket(fd);
                    if (newfd <= 0){
                        continue;
                    }
                    cbInfo.data.assign((const char*)&newfd, sizeof(newfd));
                }
                else{
                    cbInfo.eventType = IOEVENT_RECV;
                    if (SocketOp::readAll(fd, cbInfo.data) < 0){
                        cbInfo.eventType = IOEVENT_BROKEN;
                        socketInfo.bBroken = true;
                    }
                }
                listCallBack.push_back(cbInfo);
            }

            if(cur_ev.events & EPOLLOUT)
            {
                while (socketInfo.sendBuffer.empty() == false)
                {
                    std::string& toSend = socketInfo.sendBuffer.front();
                    int nsend = SocketOp::sendAll(fd, toSend);
                    if (nsend < 0){
                        socketInfo.sendBuffer.clear();
                        cbInfo.eventType = IOEVENT_BROKEN;
                        listCallBack.push_back(cbInfo);
                        
                        socketInfo.bBroken = true;
                        break;
                    }
                    else if (nsend == (int)toSend.size())
                    {
                        socketInfo.sendBuffer.pop_front();
                    }
                    else{
                        toSend = toSend.substr(nsend);
                        break;
                    }
                }
            }

            if (cur_ev.events & (EPOLLERR | EPOLLHUP))
            {
                cbInfo.eventType = IOEVENT_BROKEN;
                listCallBack.push_back(cbInfo);
            }
        }
    }
    for (size_t i = 0; i < listCallBack.size(); ++i)
    {
        IOCallBackInfo& info = listCallBack[i];
        info.eventHandler(info.fd, info.eventType, info.data.c_str(), info.data.size());
    }
    for (size_t i = 0; i < listFunc.size(); ++i)
    {
        listFunc[i]();
    }
    listFunc.clear();
    return 0;
}
int IOEventEpoll::stop()
{
    if (m_running)
    {
        m_running = false;
        //printf("IOEventEpoll::stop begin!\n");
        notify();
    }

    return 0;
}
int IOEventEpoll::regfd(Socketfd fd, IOEventFunc eventHandler)
{
    if (!eventHandler){
        return -1;
    }
    SocketOp::set_no_delay(fd);
    SocketOp::set_nonblock(fd);
    {
        LockGuard lock(m_mutex);
        IOInfo& info = m_allIOinfo[fd];
        info.eventHandler = eventHandler;
    }
    
    struct epoll_event ee = { 0, { 0 } };
    ee.data.fd   = fd;
    ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;

    //printf("IOEventEpoll::regfd fd=%d\n", fd);
    return ::epoll_ctl(m_efd, EPOLL_CTL_ADD, fd, &ee);
}
int IOEventEpoll::regfdAccept(Socketfd fd, IOEventFunc eventHandler)
{
    if (!eventHandler){
        return -1;
    }
    SocketOp::set_no_delay(fd);
    SocketOp::set_nonblock(fd);
    {
        LockGuard lock(m_mutex);
        IOInfo& info = m_allIOinfo[fd];
        info.bAccept = true;
        info.eventHandler = eventHandler;
    }
    struct epoll_event ee = { 0, { 0 } };
    ee.data.fd   = fd;
    ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;
    //printf("IOEventEpoll::regfd2 fd=%d\n", fd);
    return ::epoll_ctl(m_efd, EPOLL_CTL_ADD, fd, &ee);
}
int IOEventEpoll::unregfd(Socketfd fd)
{
    {
        LockGuard lock(m_mutex);
        std::map<Socketfd, IOInfo>::iterator it = m_allIOinfo.find(fd);
        if (it == m_allIOinfo.end())
        {
            return 0;
        }
        m_listFuncToRun.push_back(funcbind(&IOEventEpoll::safeClosefd, this, fd, it->second.eventHandler));
        m_allIOinfo.erase(it);
    }
    notify();
    return 0;
}
void IOEventEpoll::asyncSend(Socketfd fd, const char* data, size_t len)
{
    {
        LockGuard lock(m_mutex);
        std::map<Socketfd, IOInfo>::iterator it = m_allIOinfo.find(fd);
        if (it == m_allIOinfo.end())
        {
            return;
        }
        IOInfo& info = it->second;
        std::string toSend(data, len);
        if (info.sendBuffer.empty()){
            int nsend = SocketOp::sendAll(fd, toSend);
            if (nsend > 0 && nsend < (int)toSend.size()){
                toSend = toSend.substr(nsend);
                info.sendBuffer.push_back(toSend);
            }
            else{
                return;
            }
        }
        else{
            info.sendBuffer.push_back(toSend);
        }
    }
    notify();
}

void IOEventEpoll::notify(){
    struct epoll_event ee = { 0, { 0 } };

	ee.data.ptr  = this;
	ee.events    = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLET;;

	epoll_ctl(m_efd, EPOLL_CTL_MOD, m_fdNotify[0], &ee);
    //printf("IOEventEpoll::notify end!\n");
}

void IOEventEpoll::safeClosefd(Socketfd fd, IOEventFunc eventHandler){
    struct epoll_event ee;
    ee.data.ptr  = (void*)0;
    ::epoll_ctl(m_efd, EPOLL_CTL_DEL, fd, &ee);
    SocketOp::close(fd);
}
void IOEventEpoll::post(FuncToRun func)
{
    LockGuard lock(m_mutex);
    m_listFuncToRun.push_back(func);
}

#endif

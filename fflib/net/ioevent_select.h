#ifndef _IO_EVENT_SELECT_H_
#define _IO_EVENT_SELECT_H_

#include "base/func.h"
#include "base/osdef.h"
#include "net/ioevent.h"
#include "base/lock.h"
#include "net/socket_op.h"

#include <map>
#include <vector>
#include <list>

namespace ff {

class IOEventSelect: IOEvent
{
public:
    struct IOInfo
    {
        IOInfo():bAccept(false){}
        bool    bAccept;
        IOEventFunc eventHandler;
        std::list<std::string> sendBuffer;
    };
    struct IOCallBackInfo
    {
        SOCKET_TYPE     fd;
        IOEVENT_TYPE    eventType;
        IOEventFunc     eventHandler;
        std::string     data;
    };
public:
    IOEventSelect():m_running(false){
        m_fdNotify = 0;
    }
    virtual ~IOEventSelect(){
        if (m_fdNotify){
            SocketOp::close(m_fdNotify);
            m_fdNotify = 0;
        }
    }

    virtual int run()
    {
        return 0;
    }
    virtual int runOnce(int ms)
    {
        struct timeval tv = {0, 0};
        tv.tv_sec  = ms/1000;
        tv.tv_usec = 1000*(ms%1000);

        const timeval *ptimeout = (ms >= 0)? &tv: NULL;

        fd_set readset;
        fd_set writeset;
        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        SOCKET_TYPE nMaxFd = 0;

        if (m_fdNotify == 0){
            m_fdNotify = socket(AF_INET, SOCK_STREAM, 0);
        }
        int nwritefds = 0;
        FD_SET(m_fdNotify, &readset);
        {
            LockGuard lock(m_mutex);
            for (std::map<SocketFd, IOInfo>::iterator it = m_allIOinfo.begin(); it != m_allIOinfo.end(); ++it){
                #ifndef _WIN32
                if (it->first >= nMaxFd){
                    nMaxFd = it->first + 1;
                }
                #endif // _WIN32
                FD_SET(it->first, &readset);
                if (it->second.sendBuffer.empty() == false){
                    FD_SET(it->first, &writeset);
                    nwritefds += 1;
                }
            }
        }

        fd_set* pwriteset = nwritefds>0? &writeset: NULL;
        int ret = select(nMaxFd, &readset, pwriteset, NULL, ptimeout);
        if (ret == 0)
        {
            // Time expired
            return ret;
        }
        else if (ret < 0){
            #ifdef _WIN32
            fprintf(stderr, "IOEventSelect::run: %d\n", WSAGetLastError());
            #endif
            return ret;;
        }

        std::vector<IOCallBackInfo> listCallBack;

        {
            LockGuard lock(m_mutex);
            #ifdef _WIN32
            for (size_t i = 0; i < readset.fd_count; i++ )
            {
                SOCKET_TYPE fd = readset.fd_array[i];
            #else
            for(SOCKET_TYPE fd = 0; fd < FD_SETSIZE; fd++)
            {
            #endif // _WIN32
                if (FD_ISSET(fd, &readset))
                {
                    std::map<SocketFd, IOInfo>::iterator it = m_allIOinfo.find(readset.fd_array[i]);
                    if (it != m_allIOinfo.end()){
                        IOCallBackInfo cbInfo;
                        cbInfo.fd = it->first;
                        cbInfo.eventHandler = it->second.eventHandler;
                        if (it->second.bAccept){
                            cbInfo.eventType = IOEVENT_ACCEPT;
                            int newfd = SocketOp::acceptSocket(it->first);
                            if (newfd <= 0){
                                continue;
                            }
                            cbInfo.data.assign((const char*)&newfd, sizeof(newfd));
                        }
                        else{
                            cbInfo.eventType = IOEVENT_READ;
                            if (SocketOp::readAll(it->first, cbInfo.data) < 0){
                                cbInfo.eventType = IOEVENT_BROKEN;
                            }
                        }
                        listCallBack.push_back(cbInfo);
                    }
                } //if
                else if (FD_ISSET(fd, &writeset)){
                    std::map<SocketFd, IOInfo>::iterator it = m_allIOinfo.find(readset.fd_array[i]);
                    if (it != m_allIOinfo.end()){
                        while (it->second.sendBuffer.empty() == false)
                        {
                            std::string& toSend = it->second.sendBuffer.front();
                            int nsend = SocketOp::sendAll(it->first, toSend);
                            if (nsend < 0){
                                it->second.sendBuffer.clear();
                                IOCallBackInfo cbInfo;
                                cbInfo.eventType = IOEVENT_BROKEN;
                                cbInfo.fd = it->first;
                                cbInfo.eventHandler = it->second.eventHandler;
                                break;
                            }
                            else if (nsend == (int)toSend.size())
                            {
                                it->second.sendBuffer.pop_front();
                            }
                            else{
                                toSend = toSend.substr(nsend);
                                break;
                            }
                        }
                    }
                }
            }//for
        }

        for (size_t i = 0; i < listCallBack.size(); ++i)
        {
            IOCallBackInfo& info = listCallBack[i];
            info.eventHandler(info.fd, info.eventType, info.data.c_str(), info.data.size());
        }

        printf("select end!\n");
        return 0;
    }
    virtual int stop()
    {
        return 0;
    }
    virtual int regfd(SOCKET_TYPE fd, IOEventFunc eventHandler)
    {
        if (!eventHandler){
            return -1;
        }
        SocketOp::set_no_delay(fd);
        {
            LockGuard lock(m_mutex);
            IOInfo& info = m_allIOinfo[fd];
            info.eventHandler = eventHandler;
        }
        notify();
        return 0;
    }
    virtual int regfdAccept(SOCKET_TYPE fd, IOEventFunc eventHandler)
    {
        if (!eventHandler){
            return -1;
        }
        SocketOp::set_no_delay(fd);
        {
            LockGuard lock(m_mutex);
            IOInfo& info = m_allIOinfo[fd];
            info.bAccept = true;
            info.eventHandler = eventHandler;
        }
        notify();
        return 0;
    }
    virtual int unregfd(SOCKET_TYPE fd)
    {
        {
            LockGuard lock(m_mutex);
            m_allIOinfo.erase(fd);
        }
        notify();
        SocketOp::close(fd);
        return 0;
    }
    virtual void asyncSend(SOCKET_TYPE fd, const char* data, size_t len)
    {
        {
            LockGuard lock(m_mutex);
            std::map<SocketFd, IOInfo>::iterator it = m_allIOinfo.find(fd);
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
            }
            else{
                info.sendBuffer.push_back(toSend);
            }
        }
        notify();
    }

    void notify(){
        if (m_fdNotify){
            SocketOp::close(m_fdNotify);
            m_fdNotify = 0;
        }
    }
protected:
    volatile bool               m_running;
    Mutex                       m_mutex;
    std::map<SocketFd, IOInfo>  m_allIOinfo;
    SocketFd                    m_fdNotify;

};

}
#endif

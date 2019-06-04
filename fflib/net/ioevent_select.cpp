
#include "base/func.h"
#include "base/osdef.h"
#include "net/ioevent.h"
#include "net/ioevent_select.h"
#include "base/lock.h"
#include "net/socket_op.h"

#include <map>
#include <vector>
#include <list>


using namespace std;
using namespace ff;

IOEventSelect::IOEventSelect():m_running(false){
    m_fdNotify[0] = 0;
    m_fdNotify[1] = 0;
#ifndef _WIN32
    ::socketpair(AF_LOCAL, SOCK_STREAM, 0, m_fdNotify);
#else
    m_fdNotify[0] = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&m_serAddr, 0, sizeof(m_serAddr));
    m_serAddr.sin_family = AF_INET;
    m_serAddr.sin_addr.s_addr = inet_addr("127.0.0.1");; //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    static int SERVER_PORT = 17000;
    SERVER_PORT += 1;
    m_serAddr.sin_port = htons(SERVER_PORT);  //端口号，需要网络序转换

    memset(m_serAddr.sin_zero,0,8);
    int re_flag=1;
    int re_len=sizeof(int);
    setsockopt(m_fdNotify[0],SOL_SOCKET,SO_REUSEADDR,(const char*)&re_flag,re_len);
    int ret = bind(m_fdNotify[0], (struct sockaddr*)&m_serAddr, sizeof(struct sockaddr));

    if(ret < 0)
    {
        //printf("IOEventSelect socket bind fail!\n");
        return;
    }
    m_fdNotify[1] = socket(AF_INET, SOCK_DGRAM, 0);
    if(connect(m_fdNotify[1],(struct sockaddr*)&m_serAddr,sizeof(m_serAddr))==-1)
    {
        perror("IOEventSelect error in connecting");
        return;
    }
#endif
}
IOEventSelect::~IOEventSelect(){
    stop();

    if (m_fdNotify[0]){
        close(m_fdNotify[0]);
        close(m_fdNotify[1]);
        m_fdNotify[0] = 0;
        m_fdNotify[1] = 0;
    }
}

int IOEventSelect::run()
{
    m_running = true;
    while (m_running)
    {
        runOnce(5000);
    }
    return 0;
}
int IOEventSelect::runOnce(int ms)
{
    struct timeval tv = {0, 0};
    tv.tv_sec  = ms/1000;
    tv.tv_usec = 1000*(ms%1000);

    timeval *ptimeout = (ms >= 0)? &tv: NULL;

    fd_set readset;
    fd_set writeset;
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    Socketfd nMaxFd = m_fdNotify[0] + 1;

    int nwritefds = 0;
    FD_SET(m_fdNotify[0], &readset);
    {
        LockGuard lock(m_mutex);
        for (std::map<Socketfd, IOInfo>::iterator it = m_allIOinfo.begin(); it != m_allIOinfo.end(); ++it){
            if (it->second.bBroken){
                continue;
            }
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
    //printf("select begin\n");
    int ret = select(nMaxFd, &readset, pwriteset, NULL, ptimeout);
    //printf("select ret=%d\n", ret);
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
    // else{
    // }
    std::vector<IOCallBackInfo> listCallBack;
    std::vector<FuncToRun> listFunc;
    {
        LockGuard lock(m_mutex);
        listFunc = m_listFuncToRun;
        m_listFuncToRun.clear();
        #ifdef _WIN32
        std::set<int> allfd;
        for (size_t i = 0; i < readset.fd_count; i++ )
        {
            allfd.insert(readset.fd_array[i]);
        }
        if (pwriteset)
        {
            for (size_t i = 0; i < pwriteset->fd_count; i++ )
            {
                allfd.insert(pwriteset->fd_array[i]);
            }
        }
        for (std::set<int>::iterator itset = allfd.begin(); itset != allfd.end(); ++itset)
        {
            Socketfd fd = *itset;
        #else
        for(Socketfd fd = 0; fd < nMaxFd; fd++)
        {
        #endif // _WIN32
            if (m_fdNotify[0] == fd)
            {
                std::string data;
                SocketOp::readAll(fd, data);
                //printf("select weakup recv=%s\n", data.c_str());
                continue;
            }
            std::map<Socketfd, IOInfo>::iterator it = m_allIOinfo.find(fd);
            if (it == m_allIOinfo.end()){
                continue;
            }
            IOInfo& socketInfo = it->second;
            IOCallBackInfo cbInfo;
            cbInfo.fd = it->first;
            cbInfo.eventHandler = it->second.eventHandler;

            if (FD_ISSET(fd, &readset))
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
            } //if
            if (FD_ISSET(fd, &writeset)){
                while (socketInfo.sendBuffer.empty() == false)
                {
                    std::string& toSend = socketInfo.sendBuffer.front();
                    int nsend = SocketOp::sendAll(fd, toSend);
                    //printf("IOEventSelect::send event nsend=%d!\n", nsend);
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
        }//for
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
int IOEventSelect::stop()
{
    if (m_running)
    {
        m_running = false;
        //printf("IOEventSelect::stop begin!\n");
        notify();
    }

    return 0;
}
int IOEventSelect::regfd(Socketfd fd, IOEventFunc eventHandler)
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
    notify();
    return 0;
}
int IOEventSelect::regfdAccept(Socketfd fd, IOEventFunc eventHandler)
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
    notify();
    return 0;
}
int IOEventSelect::unregfd(Socketfd fd)
{
    {
        LockGuard lock(m_mutex);
        std::map<Socketfd, IOInfo>::iterator it = m_allIOinfo.find(fd);
        if (it == m_allIOinfo.end())
        {
            return 0;
        }
        m_listFuncToRun.push_back(funcbind(&IOEventSelect::safeClosefd, this, fd, it->second.eventHandler));
        m_allIOinfo.erase(it);
    }
    notify();
    return 0;
}
void IOEventSelect::asyncSend(Socketfd fd, const char* data, size_t len)
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

void IOEventSelect::notify(){
    SocketOp::sendAll(m_fdNotify[1], "12345678");
    //printf("IOEventSelect::notify end!\n");
}
void IOEventSelect::safeClosefd(Socketfd fd, IOEventFunc eventHandler){
    SocketOp::close(fd);
}
void IOEventSelect::post(FuncToRun func)
{
    LockGuard lock(m_mutex);
    m_listFuncToRun.push_back(func);
}

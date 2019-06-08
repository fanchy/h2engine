#include "base/osdef.h"
#ifndef _TIMER_SERVICE_H_
#define _TIMER_SERVICE_H_

#ifdef linux
#include <sys/epoll.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#endif // _WIN32
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <list>
#include <map>


#include "base/thread.h"
#include "base/lock.h"
#include "base/task_queue.h"
#include "base/smart_ptr.h"
#include "base/func.h"

namespace ff {

class TimerService
{
    struct InteruptInfo
    {
        int pair_fds[2];
        InteruptInfo()
        {
            pair_fds[0] = -1;
            pair_fds[1] = -1;
            #ifndef _WIN32
            assert(0 == ::socketpair(AF_LOCAL, SOCK_STREAM, 0, pair_fds));
            if (write(pair_fds[1], "0", 1)){
                //!just for warning
            }
            #endif // _WIN32
        }
        ~InteruptInfo()
        {
            if (pair_fds[0] && pair_fds[1]){
                close(pair_fds[0]);
                close(pair_fds[1]);
                pair_fds[0] = -1;
                pair_fds[1] = -1;
            }
        }
        int read_fd() { return pair_fds[0]; }
        int write_fd() { return pair_fds[1]; }
    };
    struct RegisterfdedInfo
    {
        RegisterfdedInfo(int64_t nowtm, int64_t dest_ms_, const Function<void()>& t_, bool is_loop_, int64_t id):
            tmStart(nowtm),
            tmEnd(dest_ms_),
            callback(t_),
            is_loop(is_loop_),
            timerID(id)
        {}
        bool checkTimeout(int64_t curms, long min_timeout)
        {
            if (curms >= tmEnd){
                return true;
            }
            if (curms < tmStart){
                tmEnd = (tmEnd - tmStart) + curms;
                tmStart = curms;
            }
            return false;
        }
        int64_t                tmStart;//!记录开始时间，这样可以正取处理测试环境改服务器时间的场景
        int64_t                tmEnd;
        Function<void()>        callback;
        bool                    is_loop;
        int64_t                timerID;
    };
    typedef std::list<RegisterfdedInfo>             RegisterfdedInfoList;
    typedef std::multimap<long, RegisterfdedInfo>   RegisterfdedInfoMap;
public:
    static TimerService& instance(){ return Singleton<TimerService>::instance(); }
    TimerService(long tick = 50):
        m_runing(true),
        m_efd(-1),
        m_min_timeout(tick),
        m_nAutoIncID(0)
    {
#ifdef linux
        m_efd = ::epoll_create(16);
#endif
        struct lambda_t
        {
            static void run(void* p_)
            {
                ((TimerService*)p_)->run();
            }
        };
        m_thread.create_thread(funcbind(&lambda_t::run, this), 1);
    }
    virtual ~TimerService()
    {

    }
    void stop()
    {
        if (m_runing)
        {
            m_runing = false;
            interupt();
            if (m_efd){
                ::close(m_efd);
            }
            m_thread.join();
            cleardata();
        }
    }
    void cleardata(){
        m_tmpRegisterfdList.clear();
        m_mapRegisterfdedStore.clear();
    }
    void loopTimer(int64_t ms_, Function<void()> func)
    {
        if (!m_runing)
        {
            return;
        }
        struct timeval tv;
        gettimeofday(&tv, NULL);
        int64_t nowtm = int64_t(tv.tv_sec)*1000 + tv.tv_usec / 1000;;
        int64_t   dest_ms = nowtm + ms_;

        LockGuard lock(m_mutex);
        m_tmpRegisterfdList.push_back(RegisterfdedInfo(nowtm, dest_ms, func, true, ++m_nAutoIncID));
    }
    void onceTimer(int64_t ms_, Function<void()> func)
    {
        if (!m_runing)
        {
            return;
        }
        struct timeval tv;
        gettimeofday(&tv, NULL);
        int64_t nowtm = int64_t(tv.tv_sec)*1000 + tv.tv_usec / 1000;;
        int64_t   dest_ms = nowtm + ms_;

        LockGuard lock(m_mutex);
        m_tmpRegisterfdList.push_back(RegisterfdedInfo(nowtm, dest_ms, func, false, ++m_nAutoIncID));
    }
#ifdef linux
    void run()
    {
        struct epoll_event ev_set[64];
        //! interupt();

        struct timeval tv;

        do
        {
            ::epoll_wait(m_efd, ev_set, 64, m_min_timeout);

            if (false == m_runing)//! cancel
            {
                break;
            }

            gettimeofday(&tv, NULL);
            int64_t curms = tv.tv_sec*1000 + tv.tv_usec / 1000;

            addNewTimer();
            processTimerCallback(curms);
        }while (true) ;
    }
#else
    void run()
    {
        struct timeval tv;
        do
        {
            usleep(m_min_timeout*1000);
            if (false == m_runing){
                break;
            }
            gettimeofday(&tv, NULL);
            addNewTimer();
            processTimerCallback(tv.tv_sec*1000 + tv.tv_usec / 1000);
        }while (true) ;
    }
#endif
private:
    void addNewTimer()
    {
        LockGuard lock(m_mutex);
        RegisterfdedInfoList::iterator it = m_tmpRegisterfdList.begin();
        for (; it != m_tmpRegisterfdList.end(); ++it)
        {
            m_mapRegisterfdedStore.insert(std::make_pair(it->tmEnd, *it));
        }
        m_tmpRegisterfdList.clear();
    }

    void interupt()
    {
#ifdef linux
        epoll_event ev = { 0, { 0 } };
        ev.events = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP;
        ::epoll_ctl(m_efd, EPOLL_CTL_ADD, m_infoInterupt.read_fd(), &ev);
#endif
    }
    void processTimerCallback(int64_t now_)
    {
        RegisterfdedInfoMap::iterator it_begin = m_mapRegisterfdedStore.begin();
        RegisterfdedInfoMap::iterator it       = it_begin;

        for (; it != m_mapRegisterfdedStore.end(); ++it)
        {
            RegisterfdedInfo& last = it->second;
            if (false == last.checkTimeout(now_, m_min_timeout))
            {
                break;
            }

            last.callback();

            if (last.is_loop)//! 如果是循环定时器，还要重新加入到定时器队列中
            {
                loopTimer(last.tmEnd - last.tmStart, last.callback);
            }
        }

        if (it != it_begin)//! some timeout
        {
            m_mapRegisterfdedStore.erase(it_begin, it);
        }
    }

private:
    volatile bool               m_runing;
    int                         m_efd;
    volatile long               m_min_timeout;
    RegisterfdedInfoList        m_tmpRegisterfdList;
    RegisterfdedInfoMap         m_mapRegisterfdedStore;
    InteruptInfo                m_infoInterupt;
    Thread                      m_thread;
    Mutex                       m_mutex;
    int64_t                    m_nAutoIncID;
};
class Timer
{
public:
    struct TimerCB
    {
        static void callback(SharedPtr<TaskQueue> tq, Function<void()> func)
        {
            //printf( "TimerCB %p %s %d\n", tq.get(), __FILE__, __LINE__);
            tq->post(func);
            //printf( "TimerCB %p %s %d\n", tq.get(), __FILE__, __LINE__);
        }
    };
    Timer(SharedPtr<TaskQueue>& tq_):
        m_tq(tq_)
    {
    }
    void setTQ(SharedPtr<TaskQueue> tq){ m_tq = tq;}
    void loopTimer(int64_t ms_, Function<void()> func)
    {
        if (m_tq){
            TimerService::instance().loopTimer(ms_, funcbind(&TimerCB::callback, m_tq, func));
        }
        else{
            TimerService::instance().loopTimer(ms_, func);
        }
    }
    void onceTimer(int64_t ms_, Function<void()> func)
    {
        if (m_tq){
            TimerService::instance().onceTimer(ms_, funcbind(&TimerCB::callback, m_tq, func));
        }
        else{
            TimerService::instance().onceTimer(ms_, func);
        }
    }
private:
    SharedPtr<TaskQueue>            m_tq;
};

}

#endif


#ifndef _TIMER_SERVICE_H_
#define _TIMER_SERVICE_H_

#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>
#include <list>
#include <map>


#include "base/fftype.h"
#include "base/thread.h"
#include "base/lock.h"
#include "base/task_queue_i.h"

namespace ff {

class TimerService 
{
    struct interupt_info_t
    {
        int pair_fds[2];
        interupt_info_t()
        {
            //TODO assert(0 == ::socketpair(AF_LOCAL, SOCK_STREAM, 0, pair_fds));
            //TODO ::write(pair_fds[1], "0", 1);
        }
        ~interupt_info_t()
        {
            ::close(pair_fds[0]);
            ::close(pair_fds[1]);
        }
        int read_fd() { return pair_fds[0]; }
        int write_fd() { return pair_fds[1]; }
    };
    struct registered_info_t
    {
        registered_info_t(uint64_t ms_, uint64_t dest_ms_, const Task& t_, bool is_loop_, struct timeval& curTv):
            timeout(ms_),
            dest_tm(dest_ms_),
            callback(t_),
            is_loop(is_loop_),
            tv(curTv)
        {}
        bool is_timeout(const struct timeval& curTv)       {
			if (curTv.tv_sec > this->tv.tv_sec)
			{
				return true;
			}
			else if (curTv.tv_sec == this->tv.tv_sec && curTv.tv_usec >= this->tv.tv_usec)
			{
				return true;
			}
			return false;
		}
        uint64_t    timeout;
        uint64_t    dest_tm;
        Task      callback;
        bool        is_loop;
        struct timeval tv;
    };
    typedef std::list<registered_info_t>             registered_info_list_t;
    typedef multistd::map<long, registered_info_t>   registered_info_map_t;
public:
    TimerService(TaskQueueI* tq_ = NULL, long tick = 100):
        m_tq(tq_),
        m_runing(true),
        m_efd(-1),
        m_min_timeout(tick),
        m_cache_list(0),
        m_checking_list(1)
    {
        //TODO m_efd = ::epoll_create(16);
        
        struct lambda_t
        {
            static void run(void* p_)
            {
                ((TimerService*)p_)->run();
            }
        };
        m_thread.create_thread(Task(&lambda_t::run, this), 1);
    }
    virtual ~TimerService()
    {
        stop();
    }
    void stop()
    {
        if (m_runing)
        {
            m_runing = false;
            interupt();
            
            //::close(m_efd);
            //printf("begin .....select exit\n");
            m_thread.join();
            //printf("end .....select exit\n");
        }
    }
    void loopTimer(uint64_t ms_, Task func)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t   dest_ms = uint64_t(tv.tv_sec)*1000 + tv.tv_usec / 1000 + ms_;

        LockGuard lock(m_mutex);
        m_tmp_register_list.push_back(registered_info_t(ms_, dest_ms, func, true, tv));
    }
    void onceTimer(uint64_t ms_, Task func)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        tv.tv_sec += ms_ / 1000;
        tv.tv_usec += ms_ % 1000;
        uint64_t   dest_ms = uint64_t(tv.tv_sec)*1000 + tv.tv_usec / 1000 ;//+ ms_;
        
        LockGuard lock(m_mutex);
        m_tmp_register_list.push_back(registered_info_t(ms_, dest_ms, func, false, tv));
        
        time_t nowTm = ::time(NULL);
        //printf("onceTimer .....nowTm=%ld,tv_sec=%ld,dest_ms=%ld\n", nowTm, tv.tv_sec, dest_ms);
    }
    void timerCallback(uint64_t ms_, Task func)
    {
        onceTimer(ms_, func);
    }

    void run()
    {
		struct timeval tv = {0, 0};
		struct timeval tvtmp = {0, 0};
		tv.tv_sec  = 0;  
		tv.tv_usec = 1000*200;  
		SOCKET_TYPE tmpsock = socket(AF_INET, SOCK_STREAM, 0);
		while (m_runing)
		{     
			// We only care read event 
			fd_set tmpset;
			FD_ZERO(&tmpset);
			FD_SET(tmpsock, &tmpset);
			int ret = ::select(0, &tmpset, NULL, NULL, &tv);
			gettimeofday(&tvtmp, NULL);
            uint64_t cur_ms = tvtmp.tv_sec*1000 + tvtmp.tv_usec / 1000;
            
            add_new_timer();
            process_timerCallback(tvtmp);
            //printf("timer .....select\n");
            time_t nowTm = ::time(NULL);
	        //printf("onceTimer .....nowTm=%ld,tv_sec=%ld,dest_ms=%ld\n", nowTm, tvtmp.tv_sec, cur_ms);
			if (ret == 0) 
			{
				// Time expired 
			 	continue; 
			}
			else if (ret < 0){
				fprintf(stderr, "timer::runLoop: %d\n", WSAGetLastError());
				break;
			}
		}//while  
		//printf("timer .....select exit\n");
		#ifdef _WIN32
		::closesocket(tmpsock);
		#else
		::close(tmpsock);
		#endif
    	/*
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
            uint64_t cur_ms = tv.tv_sec*1000 + tv.tv_usec / 1000;
            
            add_new_timer();
            process_timerCallback(cur_ms);
            
        }while (true) ;*/
    }
    
private:
    void add_new_timer()
    {
        LockGuard lock(m_mutex);
        registered_info_list_t::iterator it = m_tmp_register_list.begin();
        for (; it != m_tmp_register_list.end(); ++it)
        {
            m_registered_store.insert(std::make_pair(it->dest_tm, *it));
        }
        m_tmp_register_list.clear();
    }

    void interupt()
    {
    	/*TODO
        epoll_event ev = { 0, { 0 } };
        ev.events = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP;
        
        ::epoll_ctl(m_efd, EPOLL_CTL_ADD, m_interupt_info.read_fd(), &ev);*/
    }
    void process_timerCallback(const struct timeval& now_)
    {
        registered_info_map_t::iterator it_begin = m_registered_store.begin();
        registered_info_map_t::iterator it       = it_begin;

        std::vector<registered_info_map_t::iterator> toDel;
        for (; it != m_registered_store.end(); ++it)
        {
            registered_info_t& last = it->second;
            if (false == last.is_timeout(now_))
            {
                continue;
            }
            toDel.push_back(it);
            if (m_tq){
                m_tq->post(last.callback); //! 投递到目标线程执行
            }
            else{
                last.callback.run();
            }
            
            if (last.is_loop)//! 如果是循环定时器，还要重新加入到定时器队列中
            {
                loopTimer(last.timeout, last.callback);
            }
        }
        
        for (size_t i = 0; i < toDel.size(); ++i){
            m_registered_store.erase(toDel[i]);
        }
        /*
        if (it != it_begin)//! some timeout 
        {
            m_registered_store.erase(it_begin, it);
        }*/
    }

private:
    TaskQueueI*            m_tq;
    volatile bool            m_runing;
    int                      m_efd;
    volatile long            m_min_timeout;
    int                      m_cache_list;
    int                      m_checking_list;
    registered_info_list_t   m_tmp_register_list;
    registered_info_map_t    m_registered_store;
    interupt_info_t          m_interupt_info;
    Thread                 m_thread;
    Mutex                  m_mutex;
};

}
#endif


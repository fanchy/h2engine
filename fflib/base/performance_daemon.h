#ifndef _PERFORMANCE_DEAMON_H_
#define _PERFORMANCE_DEAMON_H_

#include <string>
#include <map>
#include <fstream>
#include <iostream>


#include "base/task_queue_impl.h"
#include "base/timer_service.h"

#include "base/thread.h"
#include "base/singleton.h"

namespace ff {

#define AUTO_PERF() PerformanceDaemon_t::perf_tool_t __tmp__(__FUNCTION__)
#define PERF(m)     PerformanceDaemon_t::perf_tool_t __tmp__(m)
#define AUTO_CMD_PERF(X, Y) PerformanceDaemon_t::perf_tool_t __tmp__(X, Y)
#define PERF_MONITOR Singleton<PerformanceDaemon_t>::instance()


class PerformanceDaemon_t
{
public:
    struct perf_tool_t
    {
        perf_tool_t(const char* mod_, long arg_ = -1):
            mod(mod_),
            arg(arg_)
        {
            gettimeofday(&tm, NULL);
        }
        ~perf_tool_t()
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            long cost = (now.tv_sec - tm.tv_sec)*1000000 + (now.tv_usec - tm.tv_usec);
            Singleton<PerformanceDaemon_t>::instance().post(mod, arg, cost);
        }
        const char*    mod;
        long           arg;
        struct timeval tm;
    };
    struct perf_info_t
    {
        perf_info_t():
            max(0),
            min(2147483647),
            total(0),
            times(0)
        {}
        void clear()
        {
            max = 0;
            min = 2147483647;
            total = 0;
            times = 0;
        }
        long max;
        long min;
        long total;
        long times;
    };

    struct timer_lambda_t
    {
        static void exe(void* p_)
        {
            ((PerformanceDaemon_t*)p_)->handle_timer();
        }
        static void setup_timer(void* p_)
        {
            PerformanceDaemon_t* pd = (PerformanceDaemon_t*)p_;
            pd->get_task_queue().post(Task(&timer_lambda_t::exe, pd));
        }
    };
public:
    PerformanceDaemon_t();
    ~PerformanceDaemon_t();

    int start(const std::string& path_ = "./perf", int seconds_ = 30*60);

    int stop();

    void post(const std::string& mod_, long arg_, long us);

    void flush();

    void run();

    TaskQueue& get_task_queue() { return m_task_queue; }
public:
    void add_perf_data(const std::string& mod_, long arg_, long us);

private:
    void handle_timer();

private:
    volatile bool               m_started;
    int                         m_timeout_sec;
    std::map<std::string, perf_info_t>    m_perf_info;
    TaskQueue                m_task_queue;
    Thread                    m_thread;
    std::string                      m_path;
    TimerService*            m_timerService;
};

}

#endif //PERFORMANCE_DEAMON_H_

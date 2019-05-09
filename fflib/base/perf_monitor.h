#ifndef _PERFORMANCE_DEAMON_H_
#define _PERFORMANCE_DEAMON_H_

#include <string>
#include <map>
#include <fstream>
#include <iostream>


#include "base/task_queue.h"
#include "base/timer_service.h"

#include "base/thread.h"
#include "base/singleton.h"
#include "base/smart_ptr.h"

namespace ff {

#define AUTO_PERF() PerfMonitor::PerfTool __tmp__(__FUNCTION__)
#define PERF(m)     PerfMonitor::PerfTool __tmp__(m)
#define AUTO_CMD_PERF(X, Y) PerfMonitor::PerfTool __tmp__(X, Y)
#define PERF_MONITOR Singleton<PerfMonitor>::instance()


class PerfMonitor
{
public:
    struct PerfTool
    {
        PerfTool(const char* mod_, long arg_ = -1):
            mod(mod_),
            arg(arg_)
        {
            gettimeofday(&tm, NULL);
        }
        ~PerfTool()
        {
            struct timeval now;
            gettimeofday(&now, NULL);
            long cost = (now.tv_sec - tm.tv_sec)*1000000 + (now.tv_usec - tm.tv_usec);
            Singleton<PerfMonitor>::instance().post(mod, arg, cost);
        }
        const char*    mod;
        long           arg;
        struct timeval tm;
    };
    struct PerfInfo
    {
        PerfInfo():
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

    struct TimerLambda
    {
        static void exe(void* p_)
        {
            ((PerfMonitor*)p_)->handleTimer();
        }
        static void setupTimer(void* p_)
        {
            PerfMonitor* pd = (PerfMonitor*)p_;
            pd->getTaskQueue().post(TaskBinder::gen(&TimerLambda::exe, pd));
        }
    };
public:
    PerfMonitor();
    ~PerfMonitor();

    int start(std::string path_ = "./perf", int seconds_ = 30*60);

    int stop();

    void post(const std::string& mod_, long arg_, long us);

    void flush();

    void run();

    TaskQueue& getTaskQueue() { return *m_task_queue; }
public:
    void addPerfData(const std::string& mod_, long arg_, long us);

private:
    void handleTimer();

private:
    volatile bool                       m_started;
    int                                 m_timeout_sec;
    std::map<std::string, PerfInfo>     m_perf_info;
    SharedPtr<TaskQueue>                m_task_queue;
    std::string                         m_path;
    Timer                               m_timer;
};

}

#endif //PERFORMANCE_DEAMON_H_

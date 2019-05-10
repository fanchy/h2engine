#ifndef _TASK_QUEUE_I_
#define _TASK_QUEUE_I_

#include <list>
#include <vector>
#include <stdexcept>

#include "base/lock.h"
#include "base/fftype.h"
#include "base/task.h"
#include "base/thread.h"

namespace ff {

class TaskQueue
{
public:
    typedef std::list<Task> TaskList;
public:
    TaskQueue():
        m_flag(true),
        m_cond(m_mutex)
    {
        m_thread.create_thread(TaskBinder::gen(&TaskQueue::run, this), 1);
    }
    ~TaskQueue()
    {
        close();
    }
    void close()
    {
        if (!m_flag){
            return;
        }
        {
            LockGuard lock(m_mutex);
            m_flag = false;
            //printf( "%p %s %d\n", this, __FILE__, __LINE__);
            m_cond.broadcast();
            //printf( "%p %s %d\n", this, __FILE__, __LINE__);
        }
        
        m_thread.join();
        //printf( "%p %s %d\n", this, __FILE__, __LINE__);
    }

    void post(const Task& task_)
    {
        if (!m_flag){
            return;
        }
        LockGuard lock(m_mutex);
        bool need_sig = m_tasklist.empty();

        m_tasklist.push_back(task_);
        if (need_sig)
        {
            m_cond.signal();
        }
    }

    int   consume(TaskList& task_)
    {
        LockGuard lock(m_mutex);
        while (m_tasklist.empty())
        {
            if (false == m_flag)
            {
                return -1;
            }
            //printf( "%p %s %d\n", this, __FILE__, __LINE__);
            m_cond.wait();
            //printf( "%p %s %d\n", this, __FILE__, __LINE__);
        }

        task_ = m_tasklist;
        m_tasklist.clear();

        return 0;
    }

private:
    int run()
    {
        TaskList tlist;
        while (0 == consume(tlist))
        {
            for (TaskList::iterator it = tlist.begin(); it != tlist.end(); ++it)
            {
                (*it).run();
            }
            tlist.clear();
        }
        m_tasklist.clear();
        return 0;
    }

private:
    volatile bool                   m_flag;
    TaskList                        m_tasklist;
    Mutex                           m_mutex;
    ConditionVar                    m_cond;
    Thread                          m_thread;
};


}

#endif

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

class TaskQueuePool
{
    typedef TaskQueue::TaskList TaskList;
    typedef std::vector<TaskQueue*>    task_queue_vt_t;
    static void task_func(void* pd_)
    {
        TaskQueuePool* t = (TaskQueuePool*)pd_;
        t->run();
    }
public:
    static Task gen_task(TaskQueuePool* p)
    {
        return Task(&task_func, p);
    }
public:
    TaskQueuePool(int n):
        m_index(0)
    {
        for (int i = 0; i < n; ++i)
        {
            TaskQueue* p = new TaskQueue();
            m_tqs.push_back(p);
        }
    }

    void run()
    {
        /*TaskQueue* p = NULL;
        {
            LockGuard lock(m_mutex);
            if (m_index >= (int)m_tqs.size())
            {
                throw std::runtime_error("too more thread running!!");
            }
            p = m_tqs[m_index++];
        }

        p->run();*/
    }

    ~TaskQueuePool()
    {
        task_queue_vt_t::iterator it = m_tqs.begin();
        for (; it != m_tqs.end(); ++it)
        {
            delete (*it);
        }
        m_tqs.clear();
    }

    void close()
    {
        task_queue_vt_t::iterator it = m_tqs.begin();
        for (; it != m_tqs.end(); ++it)
        {
            (*it)->close();
        }
    }

    size_t size() const { return m_tqs.size(); }

    TaskQueue* alloc(long id_)
    {
        return m_tqs[id_ %  m_tqs.size()];
    }
    TaskQueue* rand_alloc()
    {
        static unsigned long id_ = 0;
        return m_tqs[++id_ %  m_tqs.size()];
    }
private:
    Mutex               m_mutex;
    task_queue_vt_t       m_tqs;
    int                      m_index;
};


}

#endif

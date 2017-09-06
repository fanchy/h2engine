#ifndef _TASK_QUEUE_IMPL_H_
#define _TASK_QUEUE_IMPL_H_

#include<pthread.h>
#include <list>
#include <stdexcept>


#include "base/task_queue_i.h"
#include "base/lock.h"

namespace ff {

class TaskQueue: public TaskQueueI
{
public:
    TaskQueue():
        m_flag(true),
        m_cond(m_mutex)
    {
    }
    ~TaskQueue()
    {
    }
    void close()
    {
    	LockGuard lock(m_mutex);
    	m_flag = false;
    	m_cond.broadcast();
    }

    void multi_produce(const task_list_t& task_)
    {
        LockGuard lock(m_mutex);
        bool need_sig = m_tasklist.empty();

        for(task_list_t::const_iterator it = task_.begin(); it != task_.end(); ++it)
        {
            m_tasklist.push_back(*it);
        }

        if (need_sig)
        {
        	m_cond.signal();
        }
    }
    void produce(const Task& task_)
    {        
        LockGuard lock(m_mutex);
        bool need_sig = m_tasklist.empty();

        m_tasklist.push_back(task_);
        if (need_sig)
		{
			m_cond.signal();
		}
    }

    int   consume(Task& task_)
    {
        LockGuard lock(m_mutex);
        while (m_tasklist.empty())
        {
            if (false == m_flag)
            {
                return -1;
            }
            m_cond.wait();
        }

        task_ = m_tasklist.front();
        m_tasklist.pop_front();

        return 0;
    }

    int run()
    {
        Task t;
        while (0 == consume(t))
        {
            t.run();
        }
        m_tasklist.clear();
        return 0;
    }

    int consume_all(task_list_t& tasks_)
    {
        LockGuard lock(m_mutex);

        while (m_tasklist.empty())
        {
            if (false == m_flag)
            {
                return -1;
            }
            m_cond.wait();
        }

        tasks_ = m_tasklist;
        m_tasklist.clear();

        return 0;
    }

    int batch_run()
	{
    	task_list_t tasks;
    	int ret = consume_all(tasks);
		while (0 == ret)
		{
			for (task_list_t::iterator it = tasks.begin(); it != tasks.end(); ++it)
			{
				(*it).run();
			}
			tasks.clear();
			ret = consume_all(tasks);
		}
		return 0;
	}
private:
    volatile bool                   m_flag;
    task_list_t                     m_tasklist;
    Mutex                         m_mutex;
    ConditionVar                 m_cond;
};

class TaskQueuePool
{
	typedef TaskQueueI::task_list_t task_list_t;
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
    	TaskQueue* p = NULL;
    	{
			LockGuard lock(m_mutex);
			if (m_index >= (int)m_tqs.size())
			{
				throw std::runtime_error("too more thread running!!");
			}
		    p = m_tqs[m_index++];
    	}

    	p->batch_run();
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
    
    TaskQueueI* alloc(long id_)
    {
    	return m_tqs[id_ %  m_tqs.size()];
    }
    TaskQueueI* rand_alloc()
	{
    	static unsigned long id_ = 0;
		return m_tqs[++id_ %  m_tqs.size()];
	}
private:
    Mutex               m_mutex;
    task_queue_vt_t       m_tqs;
    int					  m_index;
};

}

#endif

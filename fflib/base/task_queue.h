#ifndef _TASK_QUEUE_I_
#define _TASK_QUEUE_I_

#include <list>
#include <vector>
#include <stdexcept>

#include "base/lock.h"
#include "base/fftype.h"

namespace ff {

typedef void (*TaskStaticFunc)(void*);

class TaskImplI
{
public:
    virtual ~TaskImplI(){}
    virtual void run()          = 0;
    virtual TaskImplI* fork()   = 0;
};

class TaskImpl: public TaskImplI
{
public:
    TaskImpl(TaskStaticFunc func_, void* arg_):
        m_func(func_),
        m_arg(arg_)
    {}

    virtual void run()
    {
        m_func(m_arg);
    }

    virtual TaskImplI* fork()
    {
        return new TaskImpl(m_func, m_arg);
    }

protected:
    TaskStaticFunc m_func;
    void*       m_arg;
};

struct Task
{
    static void dumy(void*){}
    Task(TaskStaticFunc f_, void* d_):
        task_impl(new TaskImpl(f_, d_))
    {
    }
    Task(TaskImplI* task_imp_):
        task_impl(task_imp_)
    {
    }
    Task(const Task& src_):
        task_impl(src_.task_impl->fork())
    {
    }
    Task()
    {
        task_impl = new TaskImpl(&Task::dumy, NULL);
    }
    ~Task()
    {
        clear();
    }
    Task& operator=(const Task& src_)
    {
        clear();
        task_impl = src_.task_impl->fork();
        return *this;
    }
    
    void run()
    {
        task_impl->run();
    }
    void clear(){
        if (task_impl){
            delete task_impl;
            task_impl = NULL;
        }
    }
    TaskImplI*    task_impl;
};


class TaskQueue
{
public:
    typedef std::list<Task> TaskList;
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

    void post(const Task& task_)
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

    int consumeAll(TaskList& tasks_)
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

    int batchRun()
	{
    	TaskList tasks;
    	int ret = consumeAll(tasks);
		while (0 == ret)
		{
			for (TaskList::iterator it = tasks.begin(); it != tasks.end(); ++it)
			{
				(*it).run();
			}
			tasks.clear();
			ret = consumeAll(tasks);
		}
		return 0;
	}
private:
    volatile bool                   m_flag;
    TaskList                        m_tasklist;
    Mutex                           m_mutex;
    ConditionVar                    m_cond;
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
    	TaskQueue* p = NULL;
    	{
			LockGuard lock(m_mutex);
			if (m_index >= (int)m_tqs.size())
			{
				throw std::runtime_error("too more thread running!!");
			}
		    p = m_tqs[m_index++];
    	}

    	p->batchRun();
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
    int					  m_index;
};
struct TaskBinder
{
    //! C function
    
    static Task gen(void (*func_)(void*), void* p_)
    {  
        Task ret(func_, p_);
        return ret;
    }
    template<typename RET>
    static Task gen(RET (*func_)(void))
    {
        struct lambda_t
        {
            static void task_func(void* p_)
            {
                (*(RET(*)(void))p_)();
            };
        };
        return Task(lambda_t::task_func, (void*)func_);
    }
    template<typename FUNCT, typename ARG1>
    static Task gen(FUNCT func_, ARG1 arg1_)
    {
        struct lambda_t: public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            lambda_t(FUNCT func_, const ARG1& arg1_):
                dest_func(func_),
                arg1(arg1_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1);
            }
        };
        return Task(new lambda_t(func_, arg1_));
    }
    template<typename FUNCT, typename ARG1, typename ARG2>
    static Task gen(FUNCT func_, ARG1 arg1_, ARG2 arg2_)
    {
        struct lambda_t: public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            lambda_t(FUNCT func_, const ARG1& arg1_, const ARG2& arg2_):
                dest_func(func_),
                arg1(arg1_),
                arg2(arg2_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1, arg2);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1, arg2);
            }
        };
        return Task(new lambda_t(func_, arg1_, arg2_));
    }
    template<typename FUNCT, typename ARG1, typename ARG2, typename ARG3>
    static Task gen(FUNCT func_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_)
    {
        struct lambda_t:public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            lambda_t(FUNCT func_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_):
                dest_func(func_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1, arg2, arg3);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1, arg2, arg3);
            }
        };
        return Task(new lambda_t(func_, arg1_, arg2_, arg3_));
    }
    template<typename FUNCT, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static Task gen(FUNCT func_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_)
    {
        struct lambda_t: public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            lambda_t(FUNCT func_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_):
                dest_func(func_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1, arg2, arg3, arg4);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1, arg2, arg3, arg4);
            }
        };
        return Task(new lambda_t(func_, arg1_, arg2_, arg3_, arg4_));
    }
    template<typename FUNCT, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    static Task gen(FUNCT func_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_, ARG5 arg5_)
    {
        struct lambda_t: public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            lambda_t(FUNCT func_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_,
                     const ARG5& arg5_):
                dest_func(func_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1, arg2, arg3, arg4, arg5);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1, arg2, arg3, arg4, arg5);
            }
        };
        return Task(new lambda_t(func_, arg1_, arg2_, arg3_, arg4_, arg5_));
    }
    template<typename FUNCT, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    static Task gen(FUNCT func_,
                       ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_,
                       ARG5 arg5_, ARG6 arg6_)
    {
        struct lambda_t: public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            typename RefTypeTraits<ARG6>::RealType arg6;
            lambda_t(FUNCT func_,
                     const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_,
                     const ARG5& arg5_, const ARG6& arg6_):
                dest_func(func_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_),
                arg6(arg6_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1, arg2, arg3, arg4, arg5, arg6);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1, arg2, arg3, arg4, arg5, arg6);
            }
        };
        return Task(new lambda_t(func_, arg1_, arg2_, arg3_, arg4_, arg5_, arg6_));
    }
    template<typename FUNCT, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
            typename ARG7>
    static Task gen(FUNCT func_,
                      ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_,
                      ARG5 arg5_, ARG6 arg6_, ARG7 arg7_)
    {
        struct lambda_t: public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            typename RefTypeTraits<ARG6>::RealType arg6;
            typename RefTypeTraits<ARG7>::RealType arg7;
            lambda_t(FUNCT func_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_,
                     const ARG5& arg5_, const ARG6& arg6_, const ARG7& arg7_):
                dest_func(func_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_),
                arg6(arg6_),
                arg7(arg7_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            }
        };
        return Task(new lambda_t(func_, arg1_, arg2_, arg3_, arg4_, arg5_, arg6_, arg7_));
    }
    template<typename FUNCT, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
            typename ARG7, typename ARG8>
    static Task gen(FUNCT func_,
                       ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_,
                       ARG5 arg5_, ARG6 arg6_, ARG7 arg7_, ARG8 arg8_)
    {
        struct lambda_t: public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            typename RefTypeTraits<ARG6>::RealType arg6;
            typename RefTypeTraits<ARG7>::RealType arg7;
            typename RefTypeTraits<ARG8>::RealType arg8;
            lambda_t(FUNCT func_,
                     const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_,
                     const ARG5& arg5_, const ARG6& arg6_, const ARG7& arg7_, const ARG8& arg8_):
                dest_func(func_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_),
                arg6(arg6_),
                arg7(arg7_),
                arg8(arg8_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
            }
        };
        return Task(new lambda_t(func_, arg1_, arg2_, arg3_, arg4_, arg5_, arg6_, arg7_, arg8_));
    }
    template<typename FUNCT, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6,
            typename ARG7, typename ARG8, typename ARG9>
    static Task gen(FUNCT func_,
                      ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_,
                      ARG5 arg5_, ARG6 arg6_, ARG7 arg7_, ARG8 arg8_, ARG9 arg9_)
    {
        struct lambda_t: public TaskImplI
        {
            FUNCT dest_func;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            typename RefTypeTraits<ARG6>::RealType arg6;
            typename RefTypeTraits<ARG7>::RealType arg7;
            typename RefTypeTraits<ARG8>::RealType arg8;
            typename RefTypeTraits<ARG9>::RealType arg9;
            lambda_t(FUNCT func_,
                     const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_,
                     const ARG5& arg5_, const ARG6& arg6_, const ARG7& arg7_, const ARG8& arg8_, const ARG9& arg9_):
                dest_func(func_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_),
                arg6(arg6_),
                arg7(arg7_),
                arg8(arg8_),
                arg9(arg9_)
            {}
            virtual void run()
            {
                (*dest_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
            }
        };
        return Task(new lambda_t(func_, arg1_, arg2_, arg3_, arg4_, arg5_, arg6_, arg7_, arg8_, arg9_));
    }
    //! class fuctions
    template<typename T, typename RET>
    static Task gen(RET (T::*func_)(void), T* obj_)
    {
        struct lambda_t:public TaskImplI
        {
            RET (T::*dest_func)(void);
            T* obj;
            lambda_t(RET (T::*func_)(void), T* obj_):
                dest_func(func_),
                obj(obj_)
            {}
            virtual void run()
            {
                (obj->*dest_func)();
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj);
            }
        };
        return Task(new lambda_t(func_, obj_));
    }
    template<typename T, typename RET, typename FARG1, typename ARG1>
    static Task gen(RET (T::*func_)(FARG1), T* obj_, ARG1 arg1_)
    {
        struct lambda_t: public TaskImplI
        {
            RET (T::*dest_func)(FARG1);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            lambda_t(RET (T::*func_)(FARG1), T* obj_, const ARG1& arg1_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_));
    }
    template<typename T, typename RET, typename FARG1, typename FARG2, typename ARG1, typename ARG2>
    static Task gen(RET (T::*func_)(FARG1, FARG2), T* obj_, ARG1 arg1_, ARG2 arg2_)
    {
        struct lambda_t:public TaskImplI
        {
            RET (T::*dest_func)(FARG1, FARG2);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            lambda_t(RET (T::*func_)(FARG1, FARG2), T* obj_, const ARG1& arg1_, const ARG2& arg2_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_),
                arg2(arg2_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1, arg2);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1, arg2);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_, arg2_));
    }
    template<typename T, typename RET, typename FARG1, typename FARG2, typename FARG3, typename ARG1, typename ARG2,
            typename ARG3>
    static Task gen(RET (T::*func_)(FARG1, FARG2, FARG3), T* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_)
    {
        struct lambda_t:public TaskImplI
        {
            RET (T::*dest_func)(FARG1, FARG2, FARG3);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            lambda_t(RET (T::*func_)(FARG1, FARG2, FARG3), T* obj_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1, arg2, arg3);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1, arg2, arg3);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_, arg2_, arg3_));
    }
    template<typename T, typename RET, typename FARG1, typename FARG2, typename FARG3, typename FARG4,
             typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static Task gen(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4), T* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_)
    {
        struct lambda_t:public TaskImplI
        {
            RET (T::*dest_func)(FARG1, FARG2, FARG3, FARG4);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            lambda_t(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4), T* obj_, const ARG1& arg1_, const ARG2& arg2_,
                     const ARG3& arg3_, const ARG4& arg4_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1, arg2, arg3, arg4);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1, arg2, arg3, arg4);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_, arg2_, arg3_, arg4_));
    }
    template<typename T, typename RET,  typename FARG1, typename FARG2, typename FARG3, typename FARG4, typename FARG5,
            typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    static Task gen(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5), T* obj_, ARG1 arg1_, ARG2 arg2_,  ARG3 arg3_, ARG4 arg4_, ARG5 arg5_)
    {
        struct lambda_t: public TaskImplI
        {
            RET (T::*dest_func)(FARG1, FARG2, FARG3, FARG4, FARG5);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            lambda_t(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5), T* obj_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_,
                     const ARG5& arg5_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1, arg2, arg3, arg4, arg5);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1, arg2, arg3, arg4, arg5);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_, arg2_, arg3_, arg4_, arg5_));
    }
    template<typename T, typename RET, typename FARG1, typename FARG2, typename FARG3, typename FARG4, typename FARG5,
             typename FARG6, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
    static Task gen(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6), T* obj_, ARG1 arg1_, ARG2 arg2_,
                      ARG3 arg3_, ARG4 arg4_, ARG5 arg5_, ARG6 arg6_)
    {
        struct lambda_t: public TaskImplI
        {
            RET (T::*dest_func)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            typename RefTypeTraits<ARG6>::RealType arg6;
            lambda_t(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6), T* obj_, const ARG1& arg1_, const ARG2& arg2_,
                     const ARG3& arg3_, const ARG4& arg4_, const ARG5& arg5_, const ARG6& arg6_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_),
                arg6(arg6_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1, arg2, arg3, arg4, arg5, arg6);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1, arg2, arg3, arg4, arg5, arg6);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_, arg2_, arg3_, arg4_, arg5_, arg6_));
    }
    template<typename T, typename RET, typename FARG1, typename FARG2, typename FARG3, typename FARG4, typename FARG5,
             typename FARG6, typename FARG7,
             typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
    static Task gen(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7), T* obj_, ARG1 arg1_, ARG2 arg2_,
                      ARG3 arg3_, ARG4 arg4_, ARG5 arg5_, ARG6 arg6_, ARG7 arg7_)
    {
        struct lambda_t: public TaskImplI
        {
            RET (T::*dest_func)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            typename RefTypeTraits<ARG6>::RealType arg6;
            typename RefTypeTraits<ARG7>::RealType arg7;
            lambda_t(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7), T* obj_, const ARG1& arg1_, const ARG2& arg2_,
                     const ARG3& arg3_, const ARG4& arg4_, const ARG5& arg5_, const ARG6& arg6_, const ARG7& arg7_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_),
                arg6(arg6_),
                arg7(arg7_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_, arg2_, arg3_, arg4_, arg5_, arg6_, arg7_));
    }
    template<typename T, typename RET, typename FARG1, typename FARG2, typename FARG3, typename FARG4, typename FARG5,
    typename FARG6, typename FARG7, typename FARG8,
    typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8>
    static Task gen(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7, FARG8), T* obj_, ARG1 arg1_, ARG2 arg2_,
                      ARG3 arg3_, ARG4 arg4_, ARG5 arg5_, ARG6 arg6_, ARG7 arg7_, ARG8 arg8_)
    {
        struct lambda_t: public TaskImplI
        {
            RET (T::*dest_func)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7, FARG8);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            typename RefTypeTraits<ARG6>::RealType arg6;
            typename RefTypeTraits<ARG7>::RealType arg7;
            typename RefTypeTraits<ARG8>::RealType arg8;
            lambda_t(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7, FARG8), T* obj_, const ARG1& arg1_, const ARG2& arg2_,
                     const ARG3& arg3_, const ARG4& arg4_, const ARG5& arg5_, const ARG6& arg6_, const ARG7& arg7_, const ARG8& arg8_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_),
                arg6(arg6_),
                arg7(arg7_),
                arg8(arg8_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_, arg2_, arg3_, arg4_, arg5_, arg6_, arg7_, arg8_));
    }
    template<typename T, typename RET, typename FARG1, typename FARG2, typename FARG3, typename FARG4, typename FARG5,
            typename FARG6, typename FARG7, typename FARG8, typename FARG9,
            typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7,
            typename ARG8, typename ARG9>
    static Task gen(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7, FARG8, FARG9), T* obj_, ARG1 arg1_, ARG2 arg2_,
                      ARG3 arg3_, ARG4 arg4_, ARG5 arg5_, ARG6 arg6_, ARG7 arg7_, ARG8 arg8_, ARG9 arg9_)
    {
        struct lambda_t: public TaskImplI
        {
            RET (T::*dest_func)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7, FARG8, FARG9);
            T* obj;
            typename RefTypeTraits<ARG1>::RealType arg1;
            typename RefTypeTraits<ARG2>::RealType arg2;
            typename RefTypeTraits<ARG3>::RealType arg3;
            typename RefTypeTraits<ARG4>::RealType arg4;
            typename RefTypeTraits<ARG5>::RealType arg5;
            typename RefTypeTraits<ARG6>::RealType arg6;
            typename RefTypeTraits<ARG7>::RealType arg7;
            typename RefTypeTraits<ARG8>::RealType arg8;
            typename RefTypeTraits<ARG9>::RealType arg9;
            lambda_t(RET (T::*func_)(FARG1, FARG2, FARG3, FARG4, FARG5, FARG6, FARG7, FARG8, FARG9), T* obj_,
                     const ARG1& arg1_, const ARG2& arg2_,
                     const ARG3& arg3_, const ARG4& arg4_, const ARG5& arg5_, const ARG6& arg6_, const ARG7& arg7_,
                     const ARG8& arg8_, const ARG9& arg9_):
                dest_func(func_),
                obj(obj_),
                arg1(arg1_),
                arg2(arg2_),
                arg3(arg3_),
                arg4(arg4_),
                arg5(arg5_),
                arg6(arg6_),
                arg7(arg7_),
                arg8(arg8_),
                arg9(arg9_)
            {}
            virtual void run()
            {
                (obj->*dest_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
            }
            virtual TaskImplI* fork()
            {
                return new lambda_t(dest_func, obj, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
            }
        };
        return Task(new lambda_t(func_, obj_, arg1_, arg2_, arg3_, arg4_, arg5_, arg6_, arg7_, arg8_, arg9_));
    }
};

}

#endif
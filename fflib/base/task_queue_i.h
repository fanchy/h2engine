#ifndef _TASK_QUEUE_I_
#define _TASK_QUEUE_I_

#include <list>
#include <vector>


namespace ff {

typedef void (*task_func_t)(void*);

class TaskImplI
{
public:
    virtual ~TaskImplI(){}
    virtual void run()          = 0;
    virtual TaskImplI* fork() = 0;
};

class TaskImpl: public TaskImplI
{
public:
    TaskImpl(task_func_t func_, void* arg_):
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
    task_func_t m_func;
    void*       m_arg;
};

struct Task
{
    static void dumy(void*){}
    Task(task_func_t f_, void* d_):
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

class TaskQueueI
{
public:
    typedef std::list<Task> task_list_t;
public:
    virtual ~TaskQueueI(){}
    virtual void close() = 0;
    virtual void produce(const Task& task_) =0;
    virtual void multi_produce(const task_list_t& task_) =0;
    virtual int  consume(Task& task_) = 0;
    virtual int  consume_all(task_list_t&) = 0;
    virtual int run() = 0;
    virtual int batch_run() = 0;
};

struct TaskBinder
{
    //! C function
    
    static Task gen(void (*func_)(void*), void* p_)
    {
        return Task(func_, p_);
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
            ARG1  arg1;
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
            ARG1 arg1;
            ARG2 arg2;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
            ARG6 arg6;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
            ARG6 arg6;
            ARG7 arg7;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
            ARG6 arg6;
            ARG7 arg7;
            ARG8 arg8;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
            ARG6 arg6;
            ARG7 arg7;
            ARG8 arg8;
            ARG9 arg9;
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
            ARG1 arg1;
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
            ARG1 arg1;
            ARG2 arg2;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
            ARG6 arg6;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
            ARG6 arg6;
            ARG7 arg7;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
            ARG6 arg6;
            ARG7 arg7;
            ARG8 arg8;
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
            ARG1 arg1;
            ARG2 arg2;
            ARG3 arg3;
            ARG4 arg4;
            ARG5 arg5;
            ARG6 arg6;
            ARG7 arg7;
            ARG8 arg8;
            ARG9 arg9;
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

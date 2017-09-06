#ifndef _NET_FACTORY_H_
#define _NET_FACTORY_H_

#include "net/acceptorimpl/acceptor_linux.h"
#include "net/acceptorimpl/acceptor_linux_gate.h"

#include "net/eventloopimpl/epoll.h"
#include "net/eventloopimpl/select.h"
#include "connector.h"
#include "base/singleton.h"
#include "base/performance_daemon.h"
#include "base/thread.h"
#include "net/codec.h"
#include "net/msg_sender.h"

namespace ff {

class NetFactory
{
public:
    struct global_data_t
    {
        volatile bool      started_flag;
        TaskQueuePool* tg;
        Thread           thread;
        #ifdef _WIN32
        Select           epoll;
        #else
        Epoll            epoll;
        #endif
        std::vector<acceptor_i*>all_acceptor;
        global_data_t():
            started_flag(false),
            tg(NULL),
            epoll()
        {
        }
        ~ global_data_t()
        {
            stop();
            for (size_t i = 0; i < all_acceptor.size(); ++i)
            {
                delete all_acceptor[i];
            }
            all_acceptor.clear();
            delete tg;
            tg = NULL;
        }
        static void run_epoll(void* e_)
        {
            global_data_t* p = (global_data_t*)e_;
            p->epoll.eventLoop();
        }
        void start(int thread_num_ = 2)
        {
            if (false == started_flag)
            {
            	#ifdef _WIN32
            	WSADATA wsaData;  
				WSAStartup(MAKEWORD(1,1),&wsaData);
            	#endif
                assert(thread_num_ > 0);
                started_flag = true;
                tg = new TaskQueuePool(thread_num_);
                thread.create_thread(Task(&run_epoll, this), 1);
                thread.create_thread(TaskQueuePool::gen_task(tg), thread_num_);
                printf("net factory start ok\n");
            }
        }
        void stop()
        {
            if (true == started_flag)
            {
                for (size_t i = 0; i < all_acceptor.size(); ++i)
                {
                    all_acceptor[i]->close();
                }

                tg->close();
                epoll.close();
                thread.join();

                started_flag = false;
            }
            #ifdef _WIN32
            WSACleanup( );
            #endif
        }
    };

    static int start(int thread_num_)
    {
        Singleton<global_data_t>::instance().start(thread_num_);
        return 0;
    }
    static int stop()
    {
        Singleton<global_data_t>::instance().stop();
        return 0;
    }
    static acceptor_i* listen(const std::string& host_, MsgHandlerI* msg_handler_)
    {
        Singleton<global_data_t>::instance().start();
        AcceptorLinux* ret = new AcceptorLinux(&(Singleton<global_data_t>::instance().epoll),
                                                   msg_handler_, 
                                                   (Singleton<global_data_t>::instance().tg));
        
        if (ret->open(host_))
        {
            delete ret;
            return NULL;
        }
        Singleton<global_data_t>::instance().all_acceptor.push_back(ret);
        return ret;
    }
    static acceptor_i* gatewayListen(const std::string& host_, MsgHandlerI* msg_handler_)
    {
        Singleton<global_data_t>::instance().start();
        AcceptorLinux* ret = new AcceptorLinuxGate(&(Singleton<global_data_t>::instance().epoll),
                                                   msg_handler_, 
                                                   (Singleton<global_data_t>::instance().tg));
        
        if (ret->open(host_))
        {
            delete ret;
            return NULL;
        }
        Singleton<global_data_t>::instance().all_acceptor.push_back(ret);
        return ret;
    }
	static acceptor_i* gatewayListen(ArgHelper& arg_helper, MsgHandlerI* msg_handler_)
    {
        Singleton<global_data_t>::instance().start();
        AcceptorLinuxGate* ret = new AcceptorLinuxGate(&(Singleton<global_data_t>::instance().epoll),
                                                   msg_handler_, 
                                                   (Singleton<global_data_t>::instance().tg));
        
        if (ret->open(arg_helper))
        {
            delete ret;
            return NULL;
        }
        Singleton<global_data_t>::instance().all_acceptor.push_back(ret);
        return ret;
    }
    
    static socket_ptr_t connect(const std::string& host_, MsgHandlerI* msg_handler_)
    {
        Singleton<global_data_t>::instance().start();
        return Connector::connect(host_, &(Singleton<global_data_t>::instance().epoll), msg_handler_,
                                    (Singleton<global_data_t>::instance().tg->rand_alloc()));
    }
};

}

#endif


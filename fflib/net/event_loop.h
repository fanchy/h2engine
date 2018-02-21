#ifndef _EVENT_LOOP_I_
#define _EVENT_LOOP_I_


namespace ff {


class Fd;
//! 事件分发器
class EventLoop
{
public:
    virtual ~EventLoop(){}

    virtual int runLoop() 		    = 0;
    virtual int close() 				= 0;
    virtual int register_fd(Fd*)      = 0;
    virtual int unregister_fd(Fd*)  	= 0;
    virtual int mod_fd(Fd*)           = 0;
};

//typedef Fd epoll_Fd;
}
#endif

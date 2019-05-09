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
    virtual int registerfd(Fd*)      = 0;
    virtual int unregisterfd(Fd*)  	= 0;
    virtual int modfd(Fd*)           = 0;
};

//typedef Fd epoll_Fd;
}
#endif

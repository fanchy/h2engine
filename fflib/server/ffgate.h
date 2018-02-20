//! 连接管理
#ifndef _FF_FFGATE_H_
#define _FF_FFGATE_H_

#include <string>
#include <map>
#include <vector>
#include <set>
#include <queue>


#include "net/msg_handler.h"
#include "base/task_queue_impl.h"
#include "base/ffslot.h"
#include "net/codec.h"
#include "base/thread.h"
#include "rpc/ffrpc.h"
#include "net/msg_sender.h"
#include "base/timer_service.h"
#include "base/arg_helper.h"

namespace ff {
#define DEFAULT_LOGIC_SERVICE "scene@0"

class FFGate: public MsgHandlerI
{
    enum limite_e
    {
        MAX_MSG_QUEUE_SIZE = 1024
    };
    struct session_data_t;
    struct client_info_t;
public:
    FFGate();
    virtual ~FFGate();
    
    int open(const std::string& broker_addr, const std::string& gate_listen, int gate_index = 0);
    int close();
    
    //! 处理连接断开
    int handleBroken(socket_ptr_t sock_);
    //! 处理消息
    int handleMsg(const Message& msg_, socket_ptr_t sock_);
    
    TaskQueueI* getTaskQueue();
public:
    int close_impl();
    //! 逻辑处理,转发消息到logic service
    int routeLogicMsg(const Message& msg_, socket_ptr_t sock_, bool first);
    //! 逻辑处理,转发消息到logic service
    int routeLogicMsgCallback(ffreq_t<RouteLogicMsg_t::out_t>& req_, const userid_t& session_id_);
    //! enter scene 回调函数
    int enterWorkerCallback(ffreq_t<SessionEnterWorker::out_t>& req_, const userid_t& session_id_);
    
    //! 改变处理client 逻辑的对应的节点
    int changeSessionLogic(ffreq_t<GateChangeLogicNode::in_t, GateChangeLogicNode::out_t>& req_);
    //! 关闭某个session socket
    int closeSession(ffreq_t<GateCloseSession::in_t, GateCloseSession::out_t>& req_);
    //! 转发消息给client
    int routeMmsgToSession(ffreq_t<GateRouteMsgToSession::in_t, GateRouteMsgToSession::out_t>& req_);
    //! 广播消息给所有的client
    int broadcastMsgToSession(ffreq_t<GateBroadcastMsgToSession::in_t, GateBroadcastMsgToSession::out_t>& req_);
    
    
    userid_t allocID();
private:
    void cleanup_session(client_info_t& client_info, socket_ptr_t sock_, bool closesend = true);
public:
    userid_t                                         m_allocID;
    int                                              m_gate_index;//!这是第几个gate，现在只有一个gate，如果以后想要有多个gate，这个要被正确的赋值
    std::string                                      m_gate_name;
    SharedPtr<FFRpc>                                 m_ffrpc;

    std::map<userid_t/*sessionid*/, client_info_t>   m_client_set;
};


struct FFGate::session_data_t
{
    session_data_t(userid_t new_id_ = 0)
    {
        ::time(&online_time);
        session_id = new_id_;
    }

    const userid_t& id() const        { return session_id;    }
    void set_id(const userid_t& s_)   { session_id = s_;      }

    userid_t session_id;
    time_t online_time;
};


struct FFGate::client_info_t
{
    client_info_t():
        sock(NULL),
        alloc_worker(DEFAULT_LOGIC_SERVICE)
    {}
    socket_ptr_t     sock;
    std::string           alloc_worker;
    std::string           group_name;
    std::queue<RouteLogicMsg_t::in_t>    request_queue;//! 请求队列，客户端有可能发送多个请求，但是服务器需要一个一个处理
};

}


#endif

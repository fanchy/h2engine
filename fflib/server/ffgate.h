//! 连接管理
#ifndef _FF_FFGATE_H_
#define _FF_FFGATE_H_

#include <string>
#include <map>
#include <vector>
#include <set>
#include <queue>

#include "base/task_queue.h"
#include "base/ffslot.h"
#include "base/thread.h"
#include "rpc/ffrpc.h"
#include "base/timer_service.h"
#include "base/arg_helper.h"

namespace ff {

class FFGate
{
    enum limite_e
    {
        MAX_MSG_QUEUE_SIZE = 1024
    };
    struct SessionData;
    struct client_info_t;
public:
    FFGate();
    virtual ~FFGate();

    int open(const std::string& broker_addr, const std::string& gate_listen, int gate_index = 0);
    int close();

    //! 处理连接断开
    int handleBroken(SocketObjPtr sock_);
    //! 处理消息
    int handleMsg(const Message& msg_, SocketObjPtr sock_);

    TaskQueue& getTaskQueue();
    void handleSocketProtocol(SocketObjPtr sock_, int eventType, const Message& msg_);
public:
    int close_impl();
    //! 逻辑处理,转发消息到logic service
    int routeLogicMsg(const Message& msg_, SocketObjPtr sock_, bool first);

    //! 改变处理client 逻辑的对应的节点
    int changeSessionLogic(RPCReq<GateChangeLogicNodeReq, EmptyMsgRet>& req_);
    //! 关闭某个session socket
    int closeSession(RPCReq<GateCloseSessionReq, EmptyMsgRet>& req_);
    //! 转发消息给client
    int routeMsgToSession(RPCReq<GateRouteMsgToSessionReq, EmptyMsgRet>& req_);
    //! 广播消息给所有的client
    int broadcastMsgToSession(RPCReq<GateBroadcastMsgToSessionReq, EmptyMsgRet>& req_);


    userid_t allocID();
private:
    void cleanup_session(client_info_t& client_info, SocketObjPtr sock_, bool closesend = true);
public:
    userid_t                                         m_allocID;
    int                                              m_gate_index;//!这是第几个gate，现在只有一个gate，如果以后想要有多个gate，这个要被正确的赋值
    std::string                                      m_gate_name;
    SharedPtr<FFRpc>                                 m_ffrpc;

    std::map<userid_t/*sessionid*/, client_info_t>   m_client_set;
};


struct FFGate::SessionData
{
    SessionData(userid_t new_id_ = 0)
    {
        ::time(&online_time);
        sessionId = new_id_;
    }

    const userid_t& id() const        { return sessionId;    }
    void set_id(const userid_t& s_)   { sessionId = s_;      }

    userid_t sessionId;
    time_t online_time;
};


struct FFGate::client_info_t
{
    client_info_t():
        sock(NULL)
    {}
    SocketObjPtr          sock;
    std::string           allocWorker;
};

}


#endif

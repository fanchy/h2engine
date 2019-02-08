
#ifndef _FF_BROKER_H_
#define _FF_BROKER_H_

#include <assert.h>
#include <string>
#include <map>
#include <set>


#include "net/msg_handler.h"
#include "base/task_queue.h"
#include "base/ffslot.h"
#include "base/thread.h"
#include "base/smart_ptr.h"
#include "net/net_factory.h"
#include "rpc/ffrpc_ops.h"
#include "base/arg_helper.h"

namespace ff
{

class FFBroker: public MsgHandler
{
    //! 每个连接都要分配一个session，用于记录该socket，对应的信息s
    struct SessionData;

    //!新版本*****************************
    //!所有的注册到此broker的节点信息
    struct RegisteredNodeInfo
    {
        //!各个节点对应的连接信息
        std::map<uint64_t/* node id*/, SocketObjPtr>     node_sockets;
        //!service对应的nodeid
        RegisterToBroker::out_t                          broker_data;
    };

public:
    FFBroker();
    virtual ~FFBroker();

    //! 当有连接断开，则被回调
    int handleBroken(SocketObjPtr sock_);
    //! 当有消息到来，被回调
    int handleMsg(const Message& msg_, SocketObjPtr sock_);
    TaskQueue* getTaskQueue();

    //! 处理其他broker或者client注册到此server
    int handleRegisterToBroker(RegisterToBroker::in_t& msg_, SocketObjPtr sock_);
    //! 处理转发消息的操作
    int handleBrokerRouteMsg(BrokerRouteMsg::in_t& msg_, SocketObjPtr sock_);
    //!处理注册到master broker的消息
    int handleRegisterMasterRet(RegisterToBroker::out_t& msg_, SocketObjPtr sock_);

    //!ff 获取节点信息
    uint64_t allocNodeId(SocketObjPtr sock_);
    //! 处理同步客户端的调用请求
    int processSyncClientReq(BrokerRouteMsg::in_t& msg_, SocketObjPtr sock_);
public:
    int open(const std::string& listen, std::string bridge_broker = "", std::string master_broker = "");
    int close();
    void docleaanup();

    //! 定时器
    TimerService& getTimer();

    //! 传递消息
    int sendToRPcNode(BrokerRouteMsg::in_t& msg_);

    int getPortCfg();
    const std::string& getHostCfg();
private:

    //! 同步给所有的节点，当前的各个节点的信息
    int syncNodeInfo(RegisterToBroker::out_t& ret_msg, SocketObjPtr sock_ = NULL);
private:
    Acceptor*                                               m_acceptor;
	//! 用于分配node id
    uint64_t 								                m_for_allocID;
    //!所有的注册到此broker的节点信息
    RegisteredNodeInfo                                      m_all_registered_info;

    //!工具类
    TaskQueue                                               m_tq;
    Thread                                                  m_thread;
    //! 用于绑定回调函数
    FFSlot                                                  m_ffslot;

    //! 本 broker的监听信息
    std::string                                             m_listen_host;
    //! broker 分配的slave node id
    TimerService                                            m_timer;
};

//! 每个连接都要分配一个session，用于记录该socket，对应的信息
struct FFBroker::SessionData
{
    SessionData(int type_ = 0, uint64_t node_ = 0, std::string s_ = ""):
        node_type(type_),
        node_id(node_),
        service_name(s_)
    {}
    uint64_t getNodeId()    const { return node_id; }
    int      getType()      const { return node_type;}
    //!节点的类型
    int                     node_type;
    //! 被分配的唯一的节点id
    uint64_t                node_id;
    //! 服务名
    std::string             service_name;
};

}

#endif

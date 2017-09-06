
#ifndef _FF_BROKER_H_
#define _FF_BROKER_H_

#include <assert.h>
#include <string>
#include <map>
#include <set>


#include "net/msg_handler.h"
#include "base/task_queue_impl.h"
#include "base/ffslot.h"
#include "base/thread.h"
#include "base/smart_ptr.h"
#include "net/net_factory.h"
#include "rpc/ffrpc_ops.h"
#include "base/arg_helper.h"

namespace ff
{

class FFBroker: public MsgHandlerI
{
    //! 每个连接都要分配一个session，用于记录该socket，对应的信息s
    struct session_data_t;

    //!新版本*****************************
    //!所有的注册到此broker的节点信息
    struct registered_node_info_t
    {
        //!各个节点对应的连接信息
        std::map<uint64_t/* node id*/, socket_ptr_t>     node_sockets;
        //!service对应的nodeid
        RegisterToBroker::out_t                 broker_data;
    };

public:
    FFBroker();
    virtual ~FFBroker();

    //! 当有连接断开，则被回调
    int handleBroken(socket_ptr_t sock_);
    //! 当有消息到来，被回调
    int handleMsg(const Message& msg_, socket_ptr_t sock_);
    TaskQueueI* getTqPtr();

    //! 处理其他broker或者client注册到此server
    int handle_regiter_to_broker(RegisterToBroker::in_t& msg_, socket_ptr_t sock_);
    //! 处理转发消息的操作
    int handle_broker_route_msg(BrokerRouteMsg::in_t& msg_, socket_ptr_t sock_);
    //!处理注册到master broker的消息
    int handle_register_master_ret(RegisterToBroker::out_t& msg_, socket_ptr_t sock_);
    
    //!ff 获取节点信息
    uint64_t alloc_node_id(socket_ptr_t sock_);
    //!本身是否是master broker
    bool is_master_broker() { return m_broker_host.empty() == true; }
    //! 处理同步客户端的调用请求
    int process_sync_client_req(BrokerRouteMsg::in_t& msg_, socket_ptr_t sock_);
public:
    int open(const std::string& listen, std::string bridge_broker = "", std::string master_broker = "");
    int close();
    void real_cleaanup();

    //! 获取任务队列对象
    TaskQueue& get_tq();
    //! 定时器
    TimerService& getTimer();
    
    //! 传递消息
    int send_to_rpc_node(BrokerRouteMsg::in_t& msg_);

    //!ff
    //! 连接到broker master
    int connect_to_master_broker();
    //! 连接到bridge broker
    int connect_to_bridge_broker();
    int get_port();
    const std::string& get_host();
private:
    //! 当有连接断开，则被回调
    //int handleBroken_impl(socket_ptr_t sock_);
    //! 当有消息到来，被回调
    //int handleMsg_impl(const Message& msg_, socket_ptr_t sock_);
    
    //! 同步给所有的节点，当前的各个节点的信息
    int sync_node_info(RegisterToBroker::out_t& ret_msg, socket_ptr_t sock_ = NULL);
private:
    acceptor_i*                             m_acceptor;
	//! 用于分配node id
    uint64_t 								m_for_allocID;
    //!本身的node id[ip_port]
    uint64_t                                m_node_id;
    //!所有的注册到此broker的节点信息
    registered_node_info_t                  m_all_registered_info;

    //!工具类
    TaskQueue                            m_tq;
    Thread                                m_thread;
    //! 用于绑定回调函数
    FFSlot                                m_ffslot;
    //! 如果本身是一个slave broker，需要连接到master broker
    socket_ptr_t                            m_master_broker_sock;
    
    //! 本 broker的监听信息
    std::string                                  m_listen_host;
    //!broker master 的host信息
    std::string                                  m_broker_host;
    //! broker 分配的slave node id
    TimerService                         m_timer;
    //!此broker归属于哪一个组
    std::string                                  m_namespace;
    //! 如果需要连接bridge broker，记录各个bridge broker的配置
    std::map<std::string, socket_ptr_t>               m_bridge_broker_config;
    //! 记录所有的namespace 注册在那些bridge broker 上, master broker 使用的数据
    std::map<std::string, std::set<socket_ptr_t> >         m_namespace2bridge;
    //! 记录namespace master broker对应的node id, brideg broker使用的数据
    std::map<std::string, uint64_t>                   m_namespace2master_nodeid;
};

//! 每个连接都要分配一个session，用于记录该socket，对应的信息
struct FFBroker::session_data_t
{
    session_data_t(int type_ = 0, uint64_t node_ = 0, std::string s_ = ""):
        node_type(type_),
        node_id(node_),
        service_name(s_)
    {}
    uint64_t get_node_id() const { return node_id; }
    int      get_type()    const { return node_type;}
    //!节点的类型
    int      node_type;
    //! 被分配的唯一的节点id
    uint64_t node_id;
    //! 服务名
    std::string   service_name;
};

}

#endif

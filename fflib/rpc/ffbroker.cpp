#include "rpc/ffbroker.h"
#include "base/log.h"
#include "base/arg_helper.h"
#include "rpc/ffrpc.h"
using namespace std;
using namespace ff;

static void route_call_reconnect(FFBroker* ffbroker_);

FFBroker::FFBroker():
    m_acceptor(NULL),
    m_for_allocID(0),
    m_node_id(0),
    m_master_broker_sock(NULL),
    m_timer(&m_tq)
{
}
FFBroker::~FFBroker()
{

}
//!ff 获取节点信息
uint64_t FFBroker::allocNodeId(SocketPtr sock_)
{
    return ++m_for_allocID;
}

const string& FFBroker::getHostCfg(){
    return m_listen_host;
}
int FFBroker::getPortCfg(){
    vector<string> vt;
    StrTool::split(m_listen_host, vt, ":");//!tcp://127.0.0.1:43210
    if (vt.size() < 3){
        return 0;
    }
    return ::atoi(vt[2].c_str());
}
int FFBroker::open(const string& listen, string bridge_broker, string master_broker)
{
    NetFactory::start(1);
    vector<string> vt;
    StrTool::split(listen, vt, ":");//!tcp://127.0.0.1:43210
    if (vt.size() < 3){
        LOGERROR((BROKER, "FFBroker::open listen failed<%s>!", listen));
        return -1;
    }
    int portbegin = ::atoi(vt[2].c_str());
    for (int port = portbegin; port < portbegin + 1000; ++port)
    {
        char buff[128] = {0};
        snprintf(buff, sizeof(buff), "tcp:%s:%d", vt[1].c_str(), port);
        
        LOGTRACE((BROKER, "FFBroker::open try<%s>", buff));
        m_acceptor = NetFactory::listen(buff, this);
        if (NULL != m_acceptor)
        {
            m_listen_host = buff;
            LOGINFO((BROKER, "FFBroker::open listen_host<%s>", m_listen_host));
            break;
        }
    }
    
    if (NULL == m_acceptor)
    {
        LOGERROR((BROKER, "FFBroker::open failed<%s>", listen));
        return -1;
    }
    //! 绑定cmd 对应的回调函数
    //!新版本
    //! 处理其他broker或者client注册到此server
    m_ffslot.bind(REGISTER_TO_BROKER_REQ, FFRpcOps::genCallBack(&FFBroker::handleRegiterToBroker, this))
            .bind(BROKER_ROUTE_MSG, FFRpcOps::genCallBack(&FFBroker::handleBrokerRouteMsg, this))
            .bind(REGISTER_TO_BROKER_RET, FFRpcOps::genCallBack(&FFBroker::handleRegisterMasterRet, this))
            .bind(SYNC_CLIENT_REQ, FFRpcOps::genCallBack(&FFBroker::processSyncClientReq, this));//! 处理同步客户端的调用请求

    //! 任务队列绑定线程
    m_thread.create_thread(TaskBinder::gen(&TaskQueue::run, &m_tq), 1);

    //!如果需要连接bride broker 读取配置连接
    if (bridge_broker.empty() == false)
    {
        string tmp_config =  bridge_broker;
        vector<string> vt;
        StrTool::split(tmp_config, vt, ",");
        for (size_t i = 0; i < vt.size(); ++i)
        {
            m_bridge_broker_config[vt[i]] = NULL;
            LOGWARN((BROKER, "FFBroker::open bridge_broker<%s>", vt[i]));
        }
        if (connectToBridgeBroker())
        {
            return -1;
        }
    }
    else if (master_broker.empty() == false)
    {
        m_broker_host = master_broker;
        if (connectToMasterBroker())
        {
            return -1;
        }
        for (int i = 0; i < 300; ++i)
        {
            if (m_node_id != 0)
            {
                break;
            }            
            usleep(10);
        }
        if (m_node_id == 0)
            return -1;
        //! 注册到master broker
    }
    else//! 内存中注册此broker
    {
        Singleton<FFRpcMemoryRoute>::instance().addNode(BROKER_MASTER_NODE_ID, this);
    }
    LOGINFO((BROKER, "FFBroker::open end ok m_node_id=%d", m_node_id));
    return 0;
}

//! 定时器
TimerService& FFBroker::getTimer()
{
    return m_timer;
}
void FFBroker::docleaanup()
{
    map<uint64_t/* node id*/, SocketPtr>::iterator it = m_all_registered_info.node_sockets.begin();
    for (; it != m_all_registered_info.node_sockets.end(); ++it)
    {
        it->second->close();
    }
    m_all_registered_info.node_sockets.clear();
}
int FFBroker::close()
{
    if (!m_acceptor)
        return 0;
    m_acceptor->close();
    m_acceptor = NULL;
    m_tq.post(TaskBinder::gen(&FFBroker::docleaanup, this));
    m_timer.stop();
    m_tq.close();
    m_thread.join();
    //usleep(100);
    return 0;
}
TaskQueueI* FFBroker::getTaskQueue()
{
    return &m_tq;
}
/*
int FFBroker::handleBroken(SocketPtr sock_)
{
    m_tq.post(TaskBinder::gen(&FFBroker::handleBroken_impl, this, sock_));
    return 0;
}
int FFBroker::handleMsg(const Message& msg_, SocketPtr sock_)
{
    m_tq.post(TaskBinder::gen(&FFBroker::handleMsg_impl, this, msg_, sock_));
    return 0;
}*/
//! 当有连接断开，则被回调
int FFBroker::handleBroken(SocketPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::handleBroken_impl begin"));
    SessionData* psession = sock_->get_data<SessionData>();
    if (NULL == psession)
    {
        sock_->safeDelete();
        LOGTRACE((BROKER, "FFBroker::handleBroken_impl no session"));
        return 0;
    }

    if (sock_ == m_master_broker_sock)
    {
        m_master_broker_sock = NULL;
        //!检测是否需要重连master broker
        m_timer.onceTimer(RECONNECT_TO_BROKER_TIMEOUT, TaskBinder::gen(&route_call_reconnect, this));
        sock_->safeDelete();
        LOGTRACE((BROKER, "FFBroker::handleBroken_impl check master reconnect"));
        return 0;
    }
    else if (BRIDGE_BROKER == psession->getType())
    {
        //! 如果需要连接bridge broker，记录各个bridge broker的配置
        for (map<string, SocketPtr>::iterator it = m_bridge_broker_config.begin(); it != m_bridge_broker_config.end(); ++it)
        {
            if (it->second == sock_)
            {
                it->second = NULL;
            }
        }
        //! 记录所有的namespace 注册在那些bridge broker 上
        for (map<string, set<SocketPtr> >::iterator it2 = m_namespace2bridge.begin(); it2 != m_namespace2bridge.end(); ++it2)
        {
            it2->second.erase(sock_);
        }
        //!检测是否需要重连master broker
        m_timer.onceTimer(RECONNECT_TO_BROKER_TIMEOUT, TaskBinder::gen(&route_call_reconnect, this));
    }
    else
    {
        m_all_registered_info.node_sockets.erase(psession->node_id);
        LOGTRACE((BROKER, "FFBroker::handleBroken_impl node_id<%u> close", psession->node_id ));
        m_all_registered_info.broker_data.service2node_id.erase(psession->service_name);
    }
    delete psession;
    sock_->set_data(NULL);
    sock_->safeDelete();
    //!广播给所有的子节点
    RegisterToBroker::out_t ret_msg = m_all_registered_info.broker_data;
    syncNodeInfo(ret_msg);
    LOGTRACE((BROKER, "FFBroker::handleBroken_impl end ok"));
    return 0;

}
//! 当有消息到来，被回调
int FFBroker::handleMsg(const Message& msg_, SocketPtr sock_)
{
    uint16_t cmd = msg_.getCmd();
    LOGTRACE((BROKER, "FFBroker::handleMsg_impl cmd<%u> begin", cmd));

    FFSlot::FFCallBack* cb = m_ffslot.get_callback(cmd);
    if (cb)
    {
        try
        {
            SlotMsgArg arg(msg_.getBody(), sock_);
            cb->exe(&arg);
            LOGTRACE((BROKER, "FFBroker::handleMsg_impl cmd<%u> end ok", cmd));
            return 0;
        }
        catch(exception& e_)
        {
            LOGERROR((BROKER, "FFBroker::handleMsg_impl exception<%s>", e_.what()));
            return -1;
        }
    }
    else
    {
        LOGERROR((BROKER, "FFBroker::handleMsg_impl cmd<%u> not supported", cmd));
        return -1;
    }
    return -1;
}


//! 处理其他broker或者client注册到此server
int FFBroker::handleRegiterToBroker(RegisterToBroker::in_t& msg_, SocketPtr sock_)
{
    LOGINFO((BROKER, "FFBroker::handleRegiterToBroker begin service_name=%s", msg_.service_name));

    SessionData* psession = sock_->get_data<SessionData>();
    if (NULL != psession)
    {
        sock_->close();
        return 0;
    }

    RegisterToBroker::out_t ret_msg;
    ret_msg.register_flag = -1;//! -1表示注册失败，0表示同步消息，1表示注册成功,

    if (isMasterBroker() == false)//!如果自己是slave broker，不做任何的处理
    {
        m_all_registered_info.node_sockets[msg_.node_id] = sock_;
        LOGINFO((BROKER, "FFBroker::handleRegiterToBroker register slave broker service_name=%s, node_id=%d", msg_.service_name, msg_.node_id));
        return 0;
    }
    
    //! 那么自己是master broker，处理来自slave 和 rpc node的注册消息
    if (SLAVE_BROKER == msg_.node_type)//! 分配slave broker 节点id，同步slave的监听信息到其他rpc node
    {
        uint64_t node_id = allocNodeId(sock_);
        SessionData* psession = new SessionData(msg_.node_type, node_id);
        psession->node_id = node_id;
        sock_->set_data(psession);

        m_all_registered_info.node_sockets[node_id] = sock_;
        //!如果是slave broker，那么host参数会被赋值
        m_all_registered_info.broker_data.slave_broker_data[msg_.host] = node_id;
        LOGTRACE((BROKER, "FFBroker::handleRegiterToBroker slave register=%s", msg_.host));

        ret_msg = m_all_registered_info.broker_data;
        ret_msg.node_id = node_id;
    }
    else if (RPC_NODE == msg_.node_type)
    {
        if (m_all_registered_info.broker_data.service2node_id.find(msg_.service_name) != m_all_registered_info.broker_data.service2node_id.end())
        {
            MsgSender::send(sock_, REGISTER_TO_BROKER_RET, FFThrift::EncodeAsString(ret_msg));
            LOGERROR((BROKER, "FFBroker::handleRegiterToBroker service<%s> exist", msg_.service_name));
            return 0;
        }

        uint64_t node_id = allocNodeId(sock_);
        SessionData* psession = new SessionData(msg_.node_type, node_id);
        psession->node_id = node_id;
        sock_->set_data(psession);

        m_all_registered_info.node_sockets[node_id] = sock_;
        if  (0 != msg_.bind_broker_id)
        {
            LOGINFO((BROKER, "FFBroker::handleRegiterToBroker service<%s> bind_broker_id=%u exist",
                                msg_.service_name, msg_.bind_broker_id));
            m_all_registered_info.broker_data.rpc_bind_broker_info[node_id] = msg_.bind_broker_id;
        }
        if (msg_.service_name.empty() == false)
        {
            psession->service_name = msg_.service_name;
            m_all_registered_info.broker_data.service2node_id[msg_.service_name] = node_id;
        }
        ret_msg = m_all_registered_info.broker_data;
        ret_msg.node_id = node_id;
    }
    else if (MASTER_BROKER == msg_.node_type)//!bridge broker接受master broker的注册
    {
        uint64_t node_id = allocNodeId(sock_);
        SessionData* psession = new SessionData(msg_.node_type, node_id);
        psession->node_id = node_id;
        sock_->set_data(psession);

        m_all_registered_info.node_sockets[node_id]   = sock_;
        m_namespace2master_nodeid[msg_.reg_namespace] = node_id;
        m_all_registered_info.broker_data.reg_namespace_list.push_back(msg_.reg_namespace);
        ret_msg = m_all_registered_info.broker_data;
        LOGINFO((BROKER, "FFBroker::handleRegiterToBroker reg_namespace=%s", msg_.reg_namespace));
    }
    else
    {
        MsgSender::send(sock_, REGISTER_TO_BROKER_RET, FFThrift::EncodeAsString(ret_msg));
        LOGERROR((BROKER, "FFBroker::handleRegiterToBroker type invalid<%d>", msg_.node_type));
        return 0;
    }
    
    //!广播给所有的子节点
    syncNodeInfo(ret_msg, sock_);
    LOGTRACE((BROKER, "FFBroker::handleRegiterToBroker end ok id=%d, type=%d", ret_msg.node_id, msg_.node_type));
    return 0;
}
//! 同步给所有的节点，当前的各个节点的信息
int FFBroker::syncNodeInfo(RegisterToBroker::out_t& ret_msg, SocketPtr sock_)
{
    //!广播给所有的子节点
    map<uint64_t/* node id*/, SocketPtr>::iterator it = m_all_registered_info.node_sockets.begin();
    for (; it != m_all_registered_info.node_sockets.end(); ++it)
    {
        SessionData* psession = it->second->get_data<SessionData>();
        if (RPC_NODE == psession->getType())//!如果没有分配对应的slavebroker，那么分配一个
        {
            map<int64_t, int64_t>::iterator it_id = m_all_registered_info.broker_data.rpc_bind_broker_info.find(
                                                        psession->getNodeId());
            if (it_id == m_all_registered_info.broker_data.rpc_bind_broker_info.end() ||
                m_all_registered_info.node_sockets.find(it_id->first) == m_all_registered_info.node_sockets.end())
            {
                if (m_all_registered_info.broker_data.slave_broker_data.empty() == false)
                {
                    int index = ret_msg.node_id % m_all_registered_info.broker_data.slave_broker_data.size();
                    map<string/*host*/, int64_t>::iterator it_broker = m_all_registered_info.broker_data.slave_broker_data.begin();
                    for (int i =0;i < index; ++i)
                    {
                        ++it_broker;
                    }
                    m_all_registered_info.broker_data.rpc_bind_broker_info[psession->getNodeId()] = it_broker->second;
                    ret_msg.rpc_bind_broker_info = m_all_registered_info.broker_data.rpc_bind_broker_info;
                }
                else
                {
                    m_all_registered_info.broker_data.rpc_bind_broker_info[psession->getNodeId()] = BROKER_MASTER_NODE_ID;//!broker master
                    ret_msg.rpc_bind_broker_info = m_all_registered_info.broker_data.rpc_bind_broker_info;
                }
            }
        }

        if (sock_ == it->second)
        {
            ret_msg.register_flag  = 1;
        }
        else
        {
            ret_msg.register_flag = 0;
        }
        MsgSender::send(it->second, REGISTER_TO_BROKER_RET, FFThrift::EncodeAsString(ret_msg));
    }
    return 0;
}

//! 处理转发消息的操作
int FFBroker::handleBrokerRouteMsg(BrokerRouteMsg::in_t& msg_, SocketPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg begin"));
    SessionData* psession = sock_->get_data<SessionData>();
    if (NULL == psession)
    {
        sock_->close();
        return 0;
    }
    if (RPC_NODE == psession->getType())
    {
        //!如果找到对应的节点，那么发给对应的节点
        sendToRPcNode(msg_);
    }
    else if (MASTER_BROKER == psession->getType())//!需要转给其他的broker master
    {
        LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg from MASTER_BROKER dest_namespace=%s", msg_.dest_namespace));
        map<string, uint64_t>::iterator it = m_namespace2master_nodeid.find(msg_.dest_namespace);
        if (it != m_namespace2master_nodeid.end())
        {
            map<uint64_t/* node id*/, SocketPtr>::iterator it2 = m_all_registered_info.node_sockets.find(it->second);
            if (it2 != m_all_registered_info.node_sockets.end())
            {
                MsgSender::send(it2->second, BROKER_ROUTE_MSG, FFThrift::EncodeAsString(msg_));
                return 0;
            }
        }
        msg_.dest_namespace.clear();
        msg_.dest_service_name.clear();
        msg_.dest_node_id = msg_.from_node_id;
        msg_.err_info = "dest_namespace named " + msg_.dest_namespace + " not exist in broker";
        MsgSender::send(sock_, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
    }
    else if (BRIDGE_BROKER == psession->getType())//!需要转给其rpc node
    {
        LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg from BRIDGE_BROKER dest_namespace=%s", msg_.dest_namespace));
        if (msg_.dest_service_name.empty() == false)
        {
            map<string, int64_t>::iterator it = m_all_registered_info.broker_data.service2node_id.find(msg_.dest_service_name);
            if (it != m_all_registered_info.broker_data.service2node_id.end())
            {
                uint64_t dest_node_id = it->second;
                msg_.dest_node_id = dest_node_id;
                map<uint64_t/* node id*/, SocketPtr>::iterator it = m_all_registered_info.node_sockets.find(msg_.dest_node_id);
                if (it != m_all_registered_info.node_sockets.end())
                {
                    MsgSender::send(it->second, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
                    LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg from BRIDGE_BROKER dest_node_id=%s", msg_.dest_node_id));
                    return 0;
                }
            }
        }
        else
        {
            map<uint64_t/* node id*/, SocketPtr>::iterator it = m_all_registered_info.node_sockets.find(msg_.dest_node_id);
            if (it != m_all_registered_info.node_sockets.end())
            {
                MsgSender::send(it->second, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
                LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg from BRIDGE_BROKER callback dest_node_id=%s", msg_.dest_node_id));
                return 0;
            }
        }
        msg_.dest_namespace = msg_.from_namespace;
        msg_.dest_service_name.clear();
        msg_.dest_node_id = msg_.from_node_id;
        msg_.err_info = "dest_namespace named " + msg_.dest_namespace + "::" + msg_.dest_service_name + " not exist in broker";
        MsgSender::send(sock_, BROKER_ROUTE_MSG, FFThrift::EncodeAsString(msg_));
        LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg from BRIDGE_BROKER dest_service_name=%s not exist", msg_.dest_service_name));
    }
    LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg end ok msg body_size=%d", msg_.body.size()));
    return 0;
}

//! 处理同步客户端的调用请求
int FFBroker::processSyncClientReq(BrokerRouteMsg::in_t& msg_, SocketPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::processSyncClientReq begin"));
    SessionData* psession = sock_->get_data<SessionData>();
    if (NULL == psession)
    {
        psession = new SessionData(SYNC_CLIENT_NODE, allocNodeId(sock_));
        sock_->set_data(psession);
        m_all_registered_info.node_sockets[psession->getNodeId()] = sock_;
    }
    msg_.from_node_id = psession->getNodeId();
    map<string, int64_t>::iterator it = m_all_registered_info.broker_data.service2node_id.find(msg_.dest_service_name);
    if (it == m_all_registered_info.broker_data.service2node_id.end())
    {
        LOGWARN((BROKER, "FFBroker::processSyncClientReq dest_service_name=%s none", msg_.dest_service_name));
        msg_.err_info = "dest_service_name named " + msg_.dest_service_name + " not exist in broker";
        MsgSender::send(sock_, BROKER_ROUTE_MSG, FFThrift::EncodeAsString(msg_));
        return 0;
    }

    msg_.dest_node_id = it->second;
    msg_.callback_id  = msg_.from_node_id;
    //!如果找到对应的节点，那么发给对应的节点
    sendToRPcNode(msg_);
    LOGTRACE((BROKER, "FFBroker::processSyncClientReq end ok service=%s from_node_id=%u",
              msg_.dest_service_name, msg_.from_node_id));
    return 0;
}

int FFBroker::sendToRPcNode(BrokerRouteMsg::in_t& msg_)
{
    //!如果找到对应的节点，那么发给对应的节点
    //!如果在内存中,那么内存间直接投递
    //!如果赋值了 dest_namespace 不为空,不等于本身的namespace 要通过bridge转发
    if (msg_.dest_namespace.empty() == false)
    {
        msg_.from_namespace = m_namespace;
        LOGTRACE((BROKER, "FFBroker::sendToRPcNode post to bridge dest_namespace=%s bodylen=%d",
                            msg_.dest_namespace, msg_.body.size()));
        set<SocketPtr>& bridge_socket = m_namespace2bridge[msg_.dest_namespace];
        if (bridge_socket.empty())
        {
            map<uint64_t/* node id*/, SocketPtr>::iterator it = m_all_registered_info.node_sockets.find(msg_.from_node_id);
            if (it != m_all_registered_info.node_sockets.end())
            {
                msg_.err_info = "dest_namespace named " + msg_.dest_namespace + " not exist in broker";
                msg_.dest_namespace.clear();
                msg_.dest_service_name.clear();
                MsgSender::send(it->second, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
            }
        }
        else
        {
            MsgSender::send(*(bridge_socket.begin()), BROKER_ROUTE_MSG, FFThrift::EncodeAsString(msg_));
        }
        return 0;
    }
    
    FFRpc* pffrpc = Singleton<FFRpcMemoryRoute>::instance().getRpc(msg_.dest_node_id);
    if (pffrpc)
    {
        LOGTRACE((BROKER, "FFBroker::sendToRPcNode memory post"));
        pffrpc->getTaskQueue()->post(TaskBinder::gen(&FFRpc::handleRpcCallMsg, pffrpc, msg_, SocketPtr(NULL)));
        return 0;
    }
    LOGINFO((BROKER, "FFBroker::sendToRPcNode dest_node=%d bodylen=%d, by socket", msg_.dest_node_id, msg_.body.size()));
    map<uint64_t/* node id*/, SocketPtr>::iterator it = m_all_registered_info.node_sockets.find(msg_.dest_node_id);
    if (it != m_all_registered_info.node_sockets.end())
    {
        MsgSender::send(it->second, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
    }
    else
    {
        it = m_all_registered_info.node_sockets.find(msg_.from_node_id);
        if (it != m_all_registered_info.node_sockets.end())
        {
            msg_.err_info = "service named " + msg_.dest_service_name + " not exist in broker";
            msg_.dest_service_name.clear();
            MsgSender::send(it->second, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
        }
        LOGERROR((BROKER, "FFBroker::handleBrokerRouteMsg end failed node=%d none exist", msg_.dest_node_id));
        return 0;
    }
    return 0;
}

//! 连接到broker master
int FFBroker::connectToMasterBroker()
{
    if (m_master_broker_sock || m_broker_host.empty() == true)
    {
        return 0;
    }
    LOGINFO((BROKER, "FFBroker::connectToMasterBroker begin<%s>", m_broker_host.c_str()));
    m_master_broker_sock = NetFactory::connect(m_broker_host, this);
    if (NULL == m_master_broker_sock)
    {
        LOGERROR((BROKER, "FFBroker::register_to_broker_master failed, can't connect to master broker<%s>", m_broker_host.c_str()));
        return -1;
    }
    SessionData* psession = new SessionData(MASTER_BROKER, BROKER_MASTER_NODE_ID);
    m_master_broker_sock->set_data(psession);

    //! 发送注册消息给master broker
    //!新版发送注册消息
    RegisterToBroker::in_t reg_msg;
    reg_msg.node_type = SLAVE_BROKER;
    reg_msg.host      = m_listen_host;
    reg_msg.node_id   = 0;
    MsgSender::send(m_master_broker_sock, REGISTER_TO_BROKER_REQ, FFThrift::EncodeAsString(reg_msg));

    LOGINFO((BROKER, "FFBroker::connectToMasterBroker ok<%s>", m_broker_host.c_str()));
    return 0;
}

//! 连接到bridge broker
int FFBroker::connectToBridgeBroker()
{
    LOGINFO((BROKER, "FFBroker::connectToBridgeBroker begin"));
    int ret = 0;
    map<string, SocketPtr>::iterator it = m_bridge_broker_config.begin();
    for (; it != m_bridge_broker_config.end(); ++it)
    {
        if (it->second)
            continue;
        const string& tmp_host = it->first;
        LOGINFO((BROKER, "FFBroker::connectToBridgeBroker try connect <%s>", tmp_host));
        
        vector<string> vt;
        StrTool::split(tmp_host, vt, "@");
        if (vt.size() != 2)
        {
            ret = -1;
            continue;
        }
        it->second = NetFactory::connect(vt[1], this);
            
        if (NULL == it->second)
        {
            LOGERROR((BROKER, "FFBroker::connectToBridgeBroker can't connect to <%s>", vt[1]));
            ret = -1;
            continue;
        }

        SessionData* psession = new SessionData(BRIDGE_BROKER);
        it->second->set_data(psession);

        //! 发送注册消息给master broker
        //!新版发送注册消息
        RegisterToBroker::in_t reg_msg;
        reg_msg.node_type = MASTER_BROKER;
        reg_msg.node_id   = 0;
        reg_msg.reg_namespace = vt[0];
        if (m_namespace.empty())
        {
            m_namespace = reg_msg.reg_namespace;
        }
        MsgSender::send(it->second, REGISTER_TO_BROKER_REQ, FFThrift::EncodeAsString(reg_msg));
    }
    LOGINFO((BROKER, "FFBroker::connectToBridgeBroker end ok"));
    return ret;
}

//!处理注册到master broker的消息
int FFBroker::handleRegisterMasterRet(RegisterToBroker::out_t& msg_, SocketPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::handleRegisterMasterRet begin"));
    SessionData* psession = sock_->get_data<SessionData>();
    if (NULL == psession)
    {
        sock_->close();
        return 0;
    }

    if (psession->getType() == MASTER_BROKER)//!注册到master broker的返回消息
    {
        if (msg_.register_flag < 0)
        {
            LOGERROR((BROKER, "FFBroker::handleRegisterMasterRet register failed, service exist"));
            return -1;
        }
        if (msg_.register_flag == 1)
        {
            m_node_id = msg_.node_id;//! -1表示注册失败，0表示同步消息，1表示注册成功
            Singleton<FFRpcMemoryRoute>::instance().addNode(m_node_id, this);
        }
        m_all_registered_info.broker_data = msg_;
    }
    else if (psession->getType() == BRIDGE_BROKER)//!注册到bridge broker的返回消息
    {
        LOGINFO((BROKER, "FFBroker::handleRegisterMasterRet BRIDGE_BROKER size=%u", msg_.reg_namespace_list.size()));
        for (size_t i = 0; i < msg_.reg_namespace_list.size(); ++i)
        {
            m_namespace2bridge[msg_.reg_namespace_list[i]].insert(sock_);
            LOGINFO((BROKER, "FFBroker::handleRegisterMasterRet BRIDGE_BROKER reg_namespace=%s", msg_.reg_namespace_list[i]));
        }
    }
    LOGINFO((BROKER, "FFBroker::handleRegisterMasterRet end ok m_node_id=%d", m_node_id));
    return 0;
}

static void reconnect_loop(FFBroker* ffbroker_)
{
    if (ffbroker_->connectToMasterBroker())
    {
        ffbroker_->getTimer().onceTimer(RECONNECT_TO_BROKER_TIMEOUT, TaskBinder::gen(&route_call_reconnect, ffbroker_));
    }
    if (ffbroker_->connectToBridgeBroker())
    {
        ffbroker_->getTimer().onceTimer(RECONNECT_TO_BROKER_TIMEOUT, TaskBinder::gen(&route_call_reconnect, ffbroker_));
    }
}
//! 投递到ffrpc 特定的线程
static void route_call_reconnect(FFBroker* ffbroker_)
{
    ffbroker_->getTaskQueue()->post(TaskBinder::gen(&reconnect_loop, ffbroker_));
}

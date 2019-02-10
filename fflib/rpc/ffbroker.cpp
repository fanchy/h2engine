#include "rpc/ffbroker.h"
#include "base/log.h"
#include "base/arg_helper.h"
#include "rpc/ffrpc.h"
using namespace std;
using namespace ff;

FFBroker::FFBroker():
    m_acceptor(NULL),
    m_for_allocID(0),
    m_timer(&m_tq)
{
}
FFBroker::~FFBroker()
{

}
//!ff 获取节点信息
uint64_t FFBroker::allocNodeId(SocketObjPtr sock_)
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
    m_ffslot.bind(REGISTER_TO_BROKER_REQ, FFRpcOps::genCallBack(&FFBroker::handleRegisterToBroker, this))
            .bind(BROKER_ROUTE_MSG, FFRpcOps::genCallBack(&FFBroker::handleBrokerRouteMsg, this))
            .bind(SYNC_CLIENT_REQ, FFRpcOps::genCallBack(&FFBroker::processSyncClientReq, this));//! 处理同步客户端的调用请求

    //! 任务队列绑定线程
    m_thread.create_thread(TaskBinder::gen(&TaskQueue::run, &m_tq), 1);

    Singleton<FFRpcMemoryRoute>::instance().addNode(BROKER_MASTER_NODE_ID, this);
    LOGINFO((BROKER, "FFBroker::open end ok"));
    return 0;
}

//! 定时器
TimerService& FFBroker::getTimer()
{
    return m_timer;
}
void FFBroker::docleaanup()
{
    map<uint64_t/* node id*/, SocketObjPtr>::iterator it = m_all_registerfded_info.node_sockets.begin();
    for (; it != m_all_registerfded_info.node_sockets.end(); ++it)
    {
        it->second->close();
    }
    m_all_registerfded_info.node_sockets.clear();
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
TaskQueue* FFBroker::getTaskQueue()
{
    return &m_tq;
}

//! 当有连接断开，则被回调
int FFBroker::handleBroken(SocketObjPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::handleBroken_impl begin"));
    SessionData* psession = sock_->getData<SessionData>();
    if (0 == psession->node_id)
    {
        LOGTRACE((BROKER, "FFBroker::handleBroken_impl no session"));
        return 0;
    }

    {
        m_all_registerfded_info.node_sockets.erase(psession->node_id);
        LOGTRACE((BROKER, "FFBroker::handleBroken_impl node_id<%u> close %u", psession->node_id, m_all_registerfded_info.node_sockets.size()));
        m_all_registerfded_info.broker_data.service2node_id.erase(psession->service_name);
    }

    //!广播给所有的子节点
    RegisterToBrokerRet ret_msg = m_all_registerfded_info.broker_data;
    syncNodeInfo(ret_msg);
    LOGTRACE((BROKER, "FFBroker::handleBroken_impl end ok"));
    return 0;

}
//! 当有消息到来，被回调
int FFBroker::handleMsg(const Message& msg_, SocketObjPtr sock_)
{
    uint16_t cmd = msg_.getCmd();
    LOGINFO((BROKER, "FFBroker::handleMsg_impl cmd<%u> ,len:%u begin", cmd, msg_.getBody().size()));

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
int FFBroker::handleRegisterToBroker(RegisterToBrokerReq& msg_, SocketObjPtr sock_)
{
    LOGINFO((BROKER, "FFBroker::handleRegisterToBroker begin service_name=%s", msg_.service_name));

    SessionData* psession = sock_->getData<SessionData>();
    if (0 != psession->node_id)
    {
        sock_->close();
        return 0;
    }

    RegisterToBrokerRet ret_msg;
    ret_msg.register_flag = -1;//! -1表示注册失败，0表示同步消息，1表示注册成功,

    if (RPC_NODE == msg_.node_type)
    {
        if (m_all_registerfded_info.broker_data.service2node_id.find(msg_.service_name) != m_all_registerfded_info.broker_data.service2node_id.end())
        {
            MsgSender::send(sock_, REGISTER_TO_BROKER_RET, FFThrift::EncodeAsString(ret_msg));
            LOGERROR((BROKER, "FFBroker::handleRegisterToBroker service<%s> exist", msg_.service_name));
            return 0;
        }

        uint64_t node_id = allocNodeId(sock_);
        //SessionData* psession = new SessionData(msg_.node_type, node_id);
        //psession->node_id = node_id;
        psession->node_type = msg_.node_type;
        psession->node_id = node_id;
        //sock_->set_data(psession);

        m_all_registerfded_info.node_sockets[node_id] = sock_;

        if (msg_.service_name.empty() == false)
        {
            psession->service_name = msg_.service_name;
            m_all_registerfded_info.broker_data.service2node_id[msg_.service_name] = node_id;
        }
        ret_msg = m_all_registerfded_info.broker_data;
        ret_msg.node_id = node_id;
    }
    else
    {
        MsgSender::send(sock_, REGISTER_TO_BROKER_RET, FFThrift::EncodeAsString(ret_msg));
        LOGERROR((BROKER, "FFBroker::handleRegisterToBroker type invalid<%d>", msg_.node_type));
        return 0;
    }

    //!广播给所有的子节点
    syncNodeInfo(ret_msg, sock_);
    LOGTRACE((BROKER, "FFBroker::handleRegisterToBroker end ok id=%d, type=%d", ret_msg.node_id, msg_.node_type));
    return 0;
}
//! 同步给所有的节点，当前的各个节点的信息
int FFBroker::syncNodeInfo(RegisterToBrokerRet& ret_msg, SocketObjPtr sock_)
{
    //!广播给所有的子节点
    map<uint64_t/* node id*/, SocketObjPtr>::iterator it = m_all_registerfded_info.node_sockets.begin();
    for (; it != m_all_registerfded_info.node_sockets.end(); ++it)
    {
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
int FFBroker::handleBrokerRouteMsg(BrokerRouteMsgReq& msg_, SocketObjPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg begin"));
    SessionData* psession = sock_->getData<SessionData>();

    if (RPC_NODE == psession->getType())
    {
        //!如果找到对应的节点，那么发给对应的节点
        sendToRPcNode(msg_);
    }
    LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg end ok msg body_size=%d", msg_.body.size()));
    return 0;
}

//! 处理同步客户端的调用请求
int FFBroker::processSyncClientReq(BrokerRouteMsgReq& msg_, SocketObjPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::processSyncClientReq begin"));
    SessionData* psession = sock_->getData<SessionData>();
    if (0 == psession->node_type)
    {
        //psession = new SessionData(SYNC_CLIENT_NODE, allocNodeId(sock_));
        psession->node_type = SYNC_CLIENT_NODE;
        psession->node_id   = allocNodeId(sock_);
        //sock_->set_data(psession);
        m_all_registerfded_info.node_sockets[psession->getNodeId()] = sock_;
    }
    msg_.from_node_id = psession->getNodeId();
    map<string, int64_t>::iterator it = m_all_registerfded_info.broker_data.service2node_id.find(msg_.dest_service_name);
    if (it == m_all_registerfded_info.broker_data.service2node_id.end())
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

int FFBroker::sendToRPcNode(BrokerRouteMsgReq& msg_)
{
    FFRpc* pffrpc = Singleton<FFRpcMemoryRoute>::instance().getRpc(msg_.dest_node_id);
    if (pffrpc)
    {
        LOGTRACE((BROKER, "FFBroker::sendToRPcNode memory post"));
        pffrpc->getTaskQueue()->post(TaskBinder::gen(&FFRpc::handleRpcCallMsg, pffrpc, msg_, SocketObjPtr(NULL)));
        return 0;
    }
    LOGINFO((BROKER, "FFBroker::sendToRPcNode dest_node=%d bodylen=%d, by socket", msg_.dest_node_id, msg_.body.size()));
    map<uint64_t/* node id*/, SocketObjPtr>::iterator it = m_all_registerfded_info.node_sockets.find(msg_.dest_node_id);
    if (it != m_all_registerfded_info.node_sockets.end())
    {
        MsgSender::send(it->second, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
    }
    else
    {
        it = m_all_registerfded_info.node_sockets.find(msg_.from_node_id);
        if (it != m_all_registerfded_info.node_sockets.end())
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

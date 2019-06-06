#include "rpc/ffbroker.h"
#include "base/log.h"
#include "base/arg_helper.h"
#include "rpc/ffrpc.h"
#include "net/ffnet.h"
using namespace std;
using namespace ff;

FFBroker::FFBroker():
    m_acceptor(NULL),
    m_for_allocID(0),
    m_tq(new TaskQueue()),
    m_timer(m_tq)
{
    //printf( "FFBroker %p %s %d\n", m_tq.get(), __FILE__, __LINE__);
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
void FFBroker::handleSocketProtocol(SocketObjPtr sock_, int eventType, const Message& msg_)
{
    if (eventType == IOEVENT_RECV){
        getTaskQueue().post(funcbind(&FFBroker::handleMsg, this, msg_, sock_));
    }
    else if (eventType == IOEVENT_BROKEN){
        getTaskQueue().post(funcbind(&FFBroker::handleBroken, this, sock_));
    }
}
int FFBroker::open(const string& listen, string bridge_broker, string master_broker)
{
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
        m_acceptor = FFNet::instance().listen(buff, funcbind(&FFBroker::handleSocketProtocol, this));
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
    m_msgHandleFunc.bind(REGISTER_TO_BROKER_REQ, FFRpcOps::genCallBack(&FFBroker::handleRegisterToBroker, this))
            .bind(BROKER_ROUTE_MSG, FFRpcOps::genCallBack(&FFBroker::handleBrokerRouteMsg, this))
            .bind(SYNC_CLIENT_REQ, FFRpcOps::genCallBack(&FFBroker::processSyncClientReq, this));//! 处理同步客户端的调用请求


    Singleton<FFRpcMemoryRoute>::instance().addNode(BROKER_MASTER_nodeId, this);
    LOGINFO((BROKER, "FFBroker::open end ok"));
    return 0;
}

//! 定时器
Timer& FFBroker::getTimer()
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
    getTaskQueue().post(funcbind(&FFBroker::docleaanup, this));

    getTaskQueue().close();

    //usleep(100);
    return 0;
}
TaskQueue& FFBroker::getTaskQueue()
{
    return *m_tq;
}

//! 当有连接断开，则被回调
int FFBroker::handleBroken(SocketObjPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::handleBroken_impl begin"));
    SessionData& psession = sock_->getData<SessionData>();
    if (0 == psession.nodeId)
    {
        LOGTRACE((BROKER, "FFBroker::handleBroken_impl no session"));
        return 0;
    }

    {
        m_all_registerfded_info.node_sockets.erase(psession.nodeId);
        LOGTRACE((BROKER, "FFBroker::handleBroken_impl nodeId<%u> close %u", psession.nodeId, m_all_registerfded_info.node_sockets.size()));
        m_all_registerfded_info.broker_data.service2nodeId.erase(psession.strServiceName);
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

    FFSlot::FFCallBack* cb = m_msgHandleFunc.get_callback(cmd);
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
    LOGINFO((BROKER, "FFBroker::handleRegisterToBroker begin strServiceName=%s", msg_.strServiceName));

    SessionData& psession = sock_->getData<SessionData>();
    if (0 != psession.nodeId)
    {
        sock_->close();
        return 0;
    }

    RegisterToBrokerRet ret_msg;
    ret_msg.registerFlag = -1;//! -1表示注册失败，0表示同步消息，1表示注册成功,

    if (RPC_NODE == msg_.nodeType)
    {
        if (m_all_registerfded_info.broker_data.service2nodeId.find(msg_.strServiceName) != m_all_registerfded_info.broker_data.service2nodeId.end())
        {
            MsgSender::send(sock_, REGISTER_TO_BROKER_RET, FFThrift::EncodeAsString(ret_msg));
            LOGERROR((BROKER, "FFBroker::handleRegisterToBroker service<%s> exist", msg_.strServiceName));
            return 0;
        }

        uint64_t nodeId = allocNodeId(sock_);
        //SessionData* psession = new SessionData(msg_.nodeType, nodeId);
        //psession.nodeId = nodeId;
        psession.nodeType = msg_.nodeType;
        psession.nodeId = nodeId;
        //sock_->set_data(psession);

        m_all_registerfded_info.node_sockets[nodeId] = sock_;

        if (msg_.strServiceName.empty() == false)
        {
            psession.strServiceName = msg_.strServiceName;
            m_all_registerfded_info.broker_data.service2nodeId[msg_.strServiceName] = nodeId;
        }
        ret_msg = m_all_registerfded_info.broker_data;
        ret_msg.nodeId = nodeId;
    }
    else
    {
        MsgSender::send(sock_, REGISTER_TO_BROKER_RET, FFThrift::EncodeAsString(ret_msg));
        LOGERROR((BROKER, "FFBroker::handleRegisterToBroker type invalid<%d>", msg_.nodeType));
        return 0;
    }

    //!广播给所有的子节点
    syncNodeInfo(ret_msg, sock_);
    LOGTRACE((BROKER, "FFBroker::handleRegisterToBroker end ok id=%d, type=%d", ret_msg.nodeId, msg_.nodeType));
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
            ret_msg.registerFlag  = 1;
        }
        else
        {
            ret_msg.registerFlag = 0;
        }
        MsgSender::send(it->second, REGISTER_TO_BROKER_RET, FFThrift::EncodeAsString(ret_msg));
    }
    return 0;
}

//! 处理转发消息的操作
int FFBroker::handleBrokerRouteMsg(BrokerRouteMsgReq& msg_, SocketObjPtr sock_)
{
    LOGTRACE((BROKER, "FFBroker::handleBrokerRouteMsg begin"));
    SessionData& psession = sock_->getData<SessionData>();

    if (RPC_NODE == psession.getType())
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
    SessionData& psession = sock_->getData<SessionData>();
    if (0 == psession.nodeType)
    {
        //psession = new SessionData(SYNC_CLIENT_NODE, allocNodeId(sock_));
        psession.nodeType = SYNC_CLIENT_NODE;
        psession.nodeId   = allocNodeId(sock_);
        //sock_->set_data(psession);
        m_all_registerfded_info.node_sockets[psession.getNodeId()] = sock_;
    }
    msg_.fromNodeId = psession.getNodeId();
    map<string, int64_t>::iterator it = m_all_registerfded_info.broker_data.service2nodeId.find(msg_.destServiceName);
    if (it == m_all_registerfded_info.broker_data.service2nodeId.end())
    {
        LOGWARN((BROKER, "FFBroker::processSyncClientReq destServiceName=%s none", msg_.destServiceName));
        msg_.errinfo = "destServiceName named " + msg_.destServiceName + " not exist in broker";
        MsgSender::send(sock_, BROKER_ROUTE_MSG, FFThrift::EncodeAsString(msg_));
        return 0;
    }

    msg_.destNodeId = it->second;
    msg_.callbackId  = msg_.fromNodeId;
    //!如果找到对应的节点，那么发给对应的节点
    sendToRPcNode(msg_);
    LOGTRACE((BROKER, "FFBroker::processSyncClientReq end ok service=%s fromNodeId=%u",
              msg_.destServiceName, msg_.fromNodeId));
    return 0;
}

int FFBroker::sendToRPcNode(BrokerRouteMsgReq& msg_)
{
    FFRpc* pffrpc = Singleton<FFRpcMemoryRoute>::instance().getRpc(msg_.destNodeId);
    if (pffrpc)
    {
        LOGTRACE((BROKER, "FFBroker::sendToRPcNode memory post"));
        Message msgData;
        msgData.getHead().cmd = BROKER_TO_CLIENT_MSG;
        string stmp = FFThrift::EncodeAsString(msg_);
        msgData.appendToBody(stmp.c_str(), stmp.size());
        pffrpc->handleSocketProtocol(NULL, IOEVENT_RECV, msgData);
        return 0;
    }
    LOGINFO((BROKER, "FFBroker::sendToRPcNode dest_node=%d bodylen=%d, by socket", msg_.destNodeId, msg_.body.size()));
    map<uint64_t/* node id*/, SocketObjPtr>::iterator it = m_all_registerfded_info.node_sockets.find(msg_.destNodeId);
    if (it != m_all_registerfded_info.node_sockets.end())
    {
        MsgSender::send(it->second, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
    }
    else
    {
        it = m_all_registerfded_info.node_sockets.find(msg_.fromNodeId);
        if (it != m_all_registerfded_info.node_sockets.end())
        {
            msg_.errinfo = "service named " + msg_.destServiceName + " not exist in broker";
            msg_.destServiceName.clear();
            MsgSender::send(it->second, BROKER_TO_CLIENT_MSG, FFThrift::EncodeAsString(msg_));
        }
        LOGERROR((BROKER, "FFBroker::handleBrokerRouteMsg end failed node=%d none exist", msg_.destNodeId));
        return 0;
    }
    return 0;
}

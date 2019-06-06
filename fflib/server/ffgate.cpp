#include "server/ffgate.h"
#include "base/log.h"
#include "net/socket_op.h"
#include "net/ffnet.h"
using namespace std;
using namespace ff;

#define FFGATE                   "FFGATE"

FFGate::FFGate():
    m_allocID(0), m_gate_index(0)
{

}
FFGate::~FFGate()
{

}
userid_t FFGate::allocID()
{
    ++m_allocID;
    if (m_gate_index == 0){
        return m_allocID;
    }

    userid_t ret = (m_allocID * 100) + m_gate_index + (m_allocID % 100);
    return ret;
}
void FFGate::handleSocketProtocol(SocketObjPtr sock_, int eventType, const Message& msg_)
{
    if (eventType == IOEVENT_RECV){
        getTaskQueue().post(funcbind(&FFGate::handleMsg, this, msg_, sock_));
    }
    else if (eventType == IOEVENT_BROKEN){
        getTaskQueue().post(funcbind(&FFGate::handleBroken, this, sock_));
    }
}
int FFGate::open(const string& broker_addr, const string& gate_listen, int gate_index)
{
    LOGINFO((FFGATE, "FFGate::open begin gate_listen<%s>", gate_listen));
    char msgbuff[128] = {0};
    snprintf(msgbuff, sizeof(msgbuff), "gate#%d", gate_index);
    m_gate_name = msgbuff;

    m_ffrpc = new FFRpc(m_gate_name);

    m_ffrpc->reg(&FFGate::changeSessionLogic, this);
    m_ffrpc->reg(&FFGate::closeSession, this);
    m_ffrpc->reg(&FFGate::routeMsgToSession, this);
    m_ffrpc->reg(&FFGate::broadcastMsgToSession, this);

    if (m_ffrpc->open(broker_addr))
    {
        LOGERROR((FFGATE, "FFGate::open failed check -broker argmuent"));
        return -1;
    }

    if (NULL == FFNet::instance().listen(gate_listen, funcbind(&FFGate::handleSocketProtocol, this)))
    {
        LOGERROR((FFGATE, "FFGate::open failed  -gate_listen %s", gate_listen));
        return -1;
    }

    LOGINFO((FFGATE, "FFGate::open end ok"));
    return 0;
}
int FFGate::close()
{
    if (m_ffrpc){
        m_ffrpc->getTaskQueue().post(funcbind(&FFGate::close_impl, this));

        m_ffrpc->close();
        for (int i = 0; i < 1000; ++i){
            if (m_ffrpc->isRunning() == false)
                break;
            usleep(1000);
        }
    }
    return 0;
}
int FFGate::close_impl()
{
    map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.begin();
    for (; it != m_client_set.end(); ++it)
    {
        it->second.sock->close();
    }
    LOGTRACE((FFGATE, "FFGate::close end ok"));
    return 0;
}

TaskQueue& FFGate::getTaskQueue()
{
    return m_ffrpc->getTaskQueue();
}

void FFGate::cleanup_session(client_info_t& client_info, SocketObjPtr sock_, bool closesend){
    SessionData& session_data = sock_->getData<SessionData>();

    if (client_info.sock == sock_)
    {
        if (closesend)
        {
            SessionOfflineReq msg;
            msg.sessionId  = session_data.id();
            m_ffrpc->call(client_info.allocWorker, msg);
        }

        m_client_set.erase(session_data.id());
    }
}
//! 处理连接断开
int FFGate::handleBroken(SocketObjPtr sock_)
{
    SessionData& session_data = sock_->getData<SessionData>();
    if (0 == session_data.id())
    {
        LOGTRACE((FFGATE, "FFGate::broken ignore %u", m_client_set.size()));
        return 0;
    }

    cleanup_session(m_client_set[session_data.id()], sock_);
    return 0;
}
//! 处理消息
int FFGate::handleMsg(const Message& msg_, SocketObjPtr sock_)
{
    SessionData& session_data = sock_->getData<SessionData>();
    if (0 == session_data.id())//! 还未验证sessionid
    {
        //!随机一个worker
        vector<string> allwoker = m_ffrpc->getServicesLike("worker#");
        if (allwoker.empty()){
            LOGERROR((FFGATE, "FFGate::handleMsg no worer exist"));
            MsgSender::send(sock_, 0, "server is busy!0x0!");
            //sock_->close();
            return 0;
        }
        session_data.sessionId = this->allocID();
        client_info_t& client_info = m_client_set[session_data.id()];
        client_info.sock           = sock_;

        client_info.allocWorker = allwoker[session_data.id() % allwoker.size()];
        LOGTRACE((FFGATE, "id:%d, client_info.allocWorker:%s", session_data.id(), client_info.allocWorker));
        return routeLogicMsg(msg_, sock_, true);
    }
    return routeLogicMsg(msg_, sock_, false);
}

//! 逻辑处理,转发消息到logic service
int FFGate::routeLogicMsg(const Message& msg_, SocketObjPtr sock_, bool first)
{
    SessionData& session_data = sock_->getData<SessionData>();
    LOGTRACE((FFGATE, "FFGate::routeLogicMsg sessionId[%ld]", session_data.id()));

    client_info_t& client_info   = m_client_set[session_data.id()];

    RouteLogicMsgReq msg;
    msg.sessionId = session_data.id();
    msg.cmd        = msg_.getCmd();
    msg.body       = msg_.getBody();
    if (first)
    {
        msg.sessionIp = SocketOp::getpeername(sock_->getRawSocket());
        LOGTRACE((FFGATE, "FFGate::handleMsg new sessionId[%ld] ip[%s] alloc[%s]",
                    session_data.id(), msg.sessionIp, client_info.allocWorker));
    }
    try{
        m_ffrpc->callSync(client_info.allocWorker, msg);//m_ffrpc->call(client_info.allocWorker, msg);
    }
    catch(exception& e){
        LOGTRACE((FFGATE, "except:%s", e.what()));
    }
    LOGTRACE((FFGATE, "FFGate::routeLogicMsg end ok allocWorker[%s] bodysize=%u", client_info.allocWorker, msg.body.size()));
    return 0;
}

//! 改变处理client 逻辑的对应的节点
int FFGate::changeSessionLogic(RPCReq<GateChangeLogicNodeReq, EmptyMsgRet>& req_)
{
    LOGTRACE((FFGATE, "FFGate::changeSessionLogic sessionId[%ld]", req_.msg.sessionId));
    map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.find(req_.msg.sessionId);
    if (it == m_client_set.end())
    {
        return 0;
    }

    EmptyMsgRet out;
    req_.response(out);

    SessionEnterWorkerReq enter_msg;
    enter_msg.fromWorker = it->second.allocWorker;

    it->second.allocWorker = req_.msg.allocWorker;

    enter_msg.sessionId = req_.msg.sessionId;
    enter_msg.fromGate = m_gate_name;
    enter_msg.sessionIp = SocketOp::getpeername(it->second.sock->getRawSocket());

    enter_msg.toWorker = req_.msg.allocWorker;
    enter_msg.extraData = req_.msg.extraData;
    m_ffrpc->call(req_.msg.allocWorker, enter_msg);


    LOGTRACE((FFGATE, "FFGate::changeSessionLogic end ok"));
    return 0;
}

//! 关闭某个session socket
int FFGate::closeSession(RPCReq<GateCloseSessionReq, EmptyMsgRet>& req_)
{
    LOGTRACE((FFGATE, "FFGate::closeSession sessionId[%ld]", req_.msg.sessionId));

    EmptyMsgRet out;
    req_.response(out);
    map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.find(req_.msg.sessionId);
    if (it == m_client_set.end())
    {
        return 0;
    }
    it->second.sock->close();
    cleanup_session(it->second, it->second.sock, false);

    LOGTRACE((FFGATE, "FFGate::GateCloseSession end ok"));
    return 0;
}

//! 转发消息给client
int FFGate::routeMsgToSession(RPCReq<GateRouteMsgToSessionReq, EmptyMsgRet>& req_)
{
    LOGTRACE((FFGATE, "FFGate::routeMsgToSession begin num[%d]", req_.msg.sessionId.size()));

    for (size_t i = 0; i < req_.msg.sessionId.size(); ++i)
    {
        userid_t& sessionId = req_.msg.sessionId[i];
        LOGTRACE((FFGATE, "FFGate::routeMsgToSession sessionId[%ld]", sessionId));

        map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.find(sessionId);
        if (it == m_client_set.end())
        {
            continue;
        }

        MsgSender::send(it->second.sock, req_.msg.cmd, req_.msg.body);
    }
    EmptyMsgRet out;
    req_.response(out);
    LOGTRACE((FFGATE, "FFGate::routeMsgToSession end ok"));
    return 0;
}

//! 广播消息给所有的client
int FFGate::broadcastMsgToSession(RPCReq<GateBroadcastMsgToSessionReq, EmptyMsgRet>& req_)
{
    LOGTRACE((FFGATE, "FFGate::broadcastMsgToSession begin"));

    map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.begin();
    for (; it != m_client_set.end(); ++it)
    {
        MsgSender::send(it->second.sock, req_.msg.cmd, req_.msg.body);
    }

    EmptyMsgRet out;
    req_.response(out);
    LOGTRACE((FFGATE, "FFGate::broadcastMsgToSession end ok"));
    return 0;
}

#include "server/ffgate.h"
#include "net/net_factory.h"
#include "base/log.h"
#include "net/socket_op.h"
#include "server/shared_mem.h"
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
    
    if (NULL == NetFactory::gatewayListen(gate_listen, this))
    {
        LOGERROR((FFGATE, "FFGate::open failed  -gate_listen %s", gate_listen));
        return -1;
    }
    
    LOGINFO((FFGATE, "FFGate::open end ok"));
    return 0;
}
int FFGate::close()
{
    if (m_ffrpc)
        m_ffrpc->getTaskQueue()->post(TaskBinder::gen(&FFGate::close_impl, this));
    return 0;
}
int FFGate::close_impl()
{
    map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.begin();
    for (; it != m_client_set.end(); ++it)
    {
        it->second.sock->close();
    }
    if (m_ffrpc)
    {
        m_ffrpc->close();
    }
    return 0;
}

TaskQueue* FFGate::getTaskQueue()
{
    return m_ffrpc->getTaskQueue();
}

void FFGate::cleanup_session(client_info_t& client_info, SocketPtr sock_, bool closesend){
    SessionData* session_data = sock_->get_data<SessionData>();
    if (NULL == session_data)
    {
        return;
    }
    if (client_info.sock == sock_)
    {
        if (closesend)
        {
            SessionOffline::in_t msg;
            msg.session_id  = session_data->id();
            m_ffrpc->call(client_info.alloc_worker, msg);
        }

        m_client_set.erase(session_data->id());
    }
    
    delete session_data;
    sock_->set_data(NULL);
}
//! 处理连接断开
int FFGate::handleBroken(SocketPtr sock_)
{
    SessionData* session_data = sock_->get_data<SessionData>();
    if (NULL == session_data)
    {
        LOGDEBUG((FFGATE, "FFGate::broken ignore"));
        sock_->safeDelete();
        return 0;
    }
    
    cleanup_session(m_client_set[session_data->id()], sock_);
    sock_->safeDelete();
    return 0;
}
//! 处理消息
int FFGate::handleMsg(const Message& msg_, SocketPtr sock_)
{
    SessionData* session_data = sock_->get_data<SessionData>();
    if (NULL == session_data)//! 还未验证sessionid
    {
        //!随机一个worker
        vector<string> allwoker = m_ffrpc->getServicesLike("worker#");
        if (allwoker.empty()){
            LOGERROR((FFGATE, "FFGate::handleMsg no worer exist"));
            MsgSender::send(sock_, 0, "server busy!");
            //sock_->close();
            return 0;
        }
        
        SessionData* session_data = new SessionData(this->allocID());
        sock_->set_data(session_data);
        client_info_t& client_info = m_client_set[session_data->id()];
        client_info.sock           = sock_;
        
        client_info.alloc_worker = allwoker[session_data->id() % allwoker.size()];
        return routeLogicMsg(msg_, sock_, true);
    }
    return routeLogicMsg(msg_, sock_, false);
}


//! enter scene 回调函数
int FFGate::enterWorkerCallback(RPCReq<SessionEnterWorker::out_t>& req_, const userid_t& session_id_)
{
    LOGTRACE((FFGATE, "FFGate::enterWorkerCallback session_id[%ld]", session_id_));
    LOGTRACE((FFGATE, "FFGate::enterWorkerCallback end ok"));
    return 0;
}

//! 逻辑处理,转发消息到logic service
int FFGate::routeLogicMsg(const Message& msg_, SocketPtr sock_, bool first)
{
    SessionData* session_data = sock_->get_data<SessionData>();
    LOGTRACE((FFGATE, "FFGate::routeLogicMsg session_id[%ld]", session_data->id()));
    
    client_info_t& client_info   = m_client_set[session_data->id()];
    
    RouteLogicMsg_t::in_t msg;
    msg.session_id = session_data->id();
    msg.cmd        = msg_.getCmd();
    msg.body       = msg_.getBody();
    if (first)
    {
        msg.session_ip = SocketOp::getpeername(sock_->socket());
        LOGTRACE((FFGATE, "FFGate::handleMsg new session_id[%ld] ip[%s] alloc[%s]", 
                    session_data->id(), msg.session_ip, client_info.alloc_worker));
    }
    m_ffrpc->call(client_info.group_name, client_info.alloc_worker, msg);
    LOGTRACE((FFGATE, "FFGate::routeLogicMsg end ok alloc_worker[%s] body=%s", client_info.alloc_worker, msg.body));
    return 0;
}

//! 改变处理client 逻辑的对应的节点
int FFGate::changeSessionLogic(RPCReq<GateChangeLogicNode::in_t, GateChangeLogicNode::out_t>& req_)
{
    LOGTRACE((FFGATE, "FFGate::changeSessionLogic session_id[%ld]", req_.msg.session_id));
    map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.find(req_.msg.session_id);
    if (it == m_client_set.end())
    {
        return 0;
    }
    
    GateChangeLogicNode::out_t out;
    req_.response(out);
    
    SessionEnterWorker::in_t enter_msg;
    it->second.group_name = req_.msg.dest_group_name;
    //enter_msg.from_group = req_.msg.cur_group_name;
    enter_msg.from_worker = it->second.alloc_worker;
    
    it->second.alloc_worker = req_.msg.alloc_worker;
    
    enter_msg.session_id = req_.msg.session_id;
    enter_msg.from_gate = m_gate_name;
    enter_msg.session_ip = SocketOp::getpeername(it->second.sock->socket());
    
    enter_msg.to_worker = req_.msg.alloc_worker;
    enter_msg.extra_data = req_.msg.extra_data;
    m_ffrpc->call(it->second.group_name, req_.msg.alloc_worker, enter_msg,
                  FFRpcOps::genCallBack(&FFGate::enterWorkerCallback, this, req_.msg.session_id));
    

    LOGTRACE((FFGATE, "FFGate::changeSessionLogic end ok"));
    return 0;
}

//! 关闭某个session socket
int FFGate::closeSession(RPCReq<GateCloseSession::in_t, GateCloseSession::out_t>& req_)
{
    LOGTRACE((FFGATE, "FFGate::closeSession session_id[%ld]", req_.msg.session_id));
    
    GateCloseSession::out_t out;
    req_.response(out);
    map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.find(req_.msg.session_id);
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
int FFGate::routeMsgToSession(RPCReq<GateRouteMsgToSession::in_t, GateRouteMsgToSession::out_t>& req_)
{
    LOGTRACE((FFGATE, "FFGate::routeMsgToSession begin num[%d]", req_.msg.session_id.size()));
    
    for (size_t i = 0; i < req_.msg.session_id.size(); ++i)
    {
        userid_t& session_id = req_.msg.session_id[i];
        LOGTRACE((FFGATE, "FFGate::routeMsgToSession session_id[%ld]", session_id));

        map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.find(session_id);
        if (it == m_client_set.end())
        {
            continue;
        }

        MsgSender::send(it->second.sock, req_.msg.cmd, req_.msg.body);
    }
    GateRouteMsgToSession::out_t out;
    req_.response(out);
    LOGTRACE((FFGATE, "FFGate::routeMsgToSession end ok"));
    return 0;
}

//! 广播消息给所有的client
int FFGate::broadcastMsgToSession(RPCReq<GateBroadcastMsgToSession::in_t, GateBroadcastMsgToSession::out_t>& req_)
{
    LOGTRACE((FFGATE, "FFGate::broadcastMsgToSession begin"));
    
    map<userid_t/*sessionid*/, client_info_t>::iterator it = m_client_set.begin();
    for (; it != m_client_set.end(); ++it)
    {
        MsgSender::send(it->second.sock, req_.msg.cmd, req_.msg.body);
    }
    
    GateBroadcastMsgToSession::out_t out;
    req_.response(out);
    LOGTRACE((FFGATE, "FFGate::broadcastMsgToSession end ok"));
    return 0;
}

#include "rpc/ffrpc.h"
#include "rpc/ffrpc_ops.h"
#include "base/log.h"
#include "rpc/ffbroker.h"
#include "net/ffnet.h"
using namespace std;
using namespace ff;

#define FFRPC                   "FFRPC"

FFRpc::FFRpc(string strServiceName_):
    m_runing(false),
    m_strServiceName(strServiceName_),
    m_nodeId(0),
    m_tq(new TaskQueue()),
    m_timer(m_tq),
    m_master_broker_sock(NULL)
{
    //printf( "FFRpc %p %s %d\n", m_tq.get(), __FILE__, __LINE__);
}

FFRpc::~FFRpc()
{
    this->close();
    LOGTRACE((FFRPC, "FFRpc::~FFRpc end"));
}
int FFRpc::open(ArgHelper& arg_helper)
{
    string arg = arg_helper.getOptionValue("-master_broker");
    if (arg.empty())
    {
        arg = arg_helper.getOptionValue("-broker");
    }
    return open(arg);
}
template<typename CLASS_TYPE, typename MSG_TYPE>
struct MsgHandleUtil
{
    static MsgHandleUtil<CLASS_TYPE, MSG_TYPE>  bind(int(CLASS_TYPE::*f)(SocketObjPtr, MSG_TYPE&), CLASS_TYPE* p){
        MsgHandleUtil<CLASS_TYPE, MSG_TYPE> ret(f, p);
        return ret;
    }
    MsgHandleUtil(int(CLASS_TYPE::*f)(SocketObjPtr, MSG_TYPE&), CLASS_TYPE* p):obj(p), func(f){}
    void operator()(SocketObjPtr s, const string& data){
        MSG_TYPE msg;
        FFThrift::DecodeFromString(msg, data);
        (obj->*(func))(s, msg);
    }
    CLASS_TYPE* obj;
    int (CLASS_TYPE::*func)(SocketObjPtr, MSG_TYPE&);
};

int FFRpc::open(const string& broker_addr)
{
    m_host = broker_addr;

    m_msgHandleFunc[REGISTER_TO_BROKER_RET] = MsgHandleUtil<FFRpc, RegisterToBrokerRet>::bind(&FFRpc::handleBrokerRegResponse, this);
    m_msgHandleFunc[BROKER_TO_CLIENT_MSG]   = MsgHandleUtil<FFRpc, BrokerRouteMsgReq>::bind(&FFRpc::handleRpcCallMsg, this);
    //!新版本
    //m_msgHandleFunc.bind(REGISTER_TO_BROKER_RET, FFRpcOps::genCallBack(&FFRpc::handleBrokerRegResponse, this))
    //        .bind(BROKER_TO_CLIENT_MSG, FFRpcOps::genCallBack(&FFRpc::handleRpcCallMsg, this));

    m_master_broker_sock = connectToBroker(m_host, BROKER_MASTER_nodeId);

    if (!m_master_broker_sock)
    {
        getTaskQueue().close();
        LOGERROR((FFRPC, "FFRpc::open failed, can't connect to remote broker<%s>", m_host.c_str()));
        return -1;
    }

    int tryTimes = 0;
    while(m_nodeId == 0)
    {
        usleep(100);
        if (!m_master_broker_sock)
        {
            LOGERROR((FFRPC, "FFRpc::open failed"));
            return -1;
        }
        ++tryTimes;
        if (tryTimes > 10000){
            if (m_master_broker_sock){
                m_master_broker_sock->close();
                m_master_broker_sock = NULL;
            }
            LOGERROR((FFRPC, "FFRpc::open failed timeout"));
            usleep(100);
            return -1;
        }
    };
    m_runing = true;
    LOGTRACE((FFRPC, "FFRpc::open end ok m_nodeId[%u]", m_nodeId));
    return 0;
}
static void CheckBrokenEnd(int* status){
    *status = 1;
}
int FFRpc::close()
{
    if (false == m_runing)
    {
        return 0;
    }
    LOGTRACE((FFRPC, "FFRpc::close 11"));

    if (m_master_broker_sock)
    {
        m_master_broker_sock->close();
    }

    volatile int status = 0;
    getTaskQueue().post(funcbind(&CheckBrokenEnd, (int*)&status));

    for (int i = 0; i < 5; ++i){
        if (1 == status)
            break;
        LOGTRACE((FFRPC, "FFRpc::close wait socket close"));
        usleep(1000);
    }
    m_runing = false;
    LOGTRACE((FFRPC, "FFRpc::close 22"));
    m_master_broker_sock = NULL;
    getTaskQueue().close();

    LOGINFO((FFRPC, "FFRpc::close end ok"));
    //usleep(100);
    return 0;
}
void FFRpc::handleSocketProtocol(SocketObjPtr sock_, int eventType, const Message& msg_)
{
    if (eventType == IOEVENT_RECV){
        if (BROKER_TO_CLIENT_MSG == (int)msg_.getCmd())//m_dataSyncCallInfo.nSyncCallBackId && 
        {
            BrokerRouteMsgReq msg;
            FFThrift::DecodeFromString(msg, msg_.getBody());
            LOGTRACE((FFRPC, "FFRpc::handleSocketProtocol begin..msg.callbackId=%ld -> %ld", m_dataSyncCallInfo.nSyncCallBackId, msg.callbackId));
            if (msg.callbackId == m_dataSyncCallInfo.nSyncCallBackId)
            {
                LockGuard lock(m_dataSyncCallInfo.mutex);

                m_dataSyncCallInfo.nSyncCallBackId = 0;
                m_dataSyncCallInfo.strResult = msg.body;
                m_dataSyncCallInfo.cond.signal();
                return;
            }
        }
        getTaskQueue().post(funcbind(&FFRpc::handleMsg, this, msg_, sock_));
    }
    else if (eventType == IOEVENT_BROKEN){
        getTaskQueue().post(funcbind(&FFRpc::handleBroken, this, sock_));
    }
}
//! 连接到broker master
SocketObjPtr FFRpc::connectToBroker(const string& host_, uint32_t nodeId_)
{
    LOGINFO((FFRPC, "FFRpc::connectToBroker begin...host_<%s>,nodeId_[%u]", host_.c_str(), nodeId_));
    SocketObjPtr sock = FFNet::instance().connect(host_, funcbind(&FFRpc::handleSocketProtocol, this));
    if (!sock)
    {
        LOGERROR((FFRPC, "FFRpc::registerfd_to_broker_master failed, can't connect to remote broker<%s>", host_.c_str()));
        return sock;
    }
    LOGTRACE((FFRPC, "FFRpc::connectToBroker trace...host_<%s>,nodeId_[%u]", host_.c_str(), nodeId_));
    SessionData& psession = sock->getData<SessionData>();
    psession.nodeId = nodeId_;

    //!新版发送注册消息
    RegisterToBrokerReq reg_msg;
    reg_msg.nodeType = RPC_NODE;
    reg_msg.strServiceName = m_strServiceName;
    string strMsg = FFThrift::EncodeAsString(reg_msg);
    MsgSender::send(sock, REGISTER_TO_BROKER_REQ, strMsg);

    LOGINFO((FFRPC, "FFRpc::connectToBroker end strMsg.size=%u", strMsg.size()));
    return sock;
}

//! 定时重连 broker master
void FFRpc::timerReconnectBroker()
{
    if (m_runing == false)
        return;

    if (!m_master_broker_sock)
    {
        m_master_broker_sock = connectToBroker(m_host, BROKER_MASTER_nodeId);
        if (!m_master_broker_sock)
        {
            LOGERROR((FFRPC, "FFRpc::timerReconnectBroker failed, can't connect to remote broker<%s>", m_host.c_str()));
            //! 设置定时器重连
            m_timer.onceTimer(RECONNECT_TO_BROKER_TIMEOUT, funcbind(&FFRpc::timerReconnectBroker, this));
        }
        else
        {
            LOGWARN((FFRPC, "FFRpc::timerReconnectBroker failed, connect to master remote broker<%s> ok", m_host.c_str()));
        }
    }
}

//! 获取任务队列对象
TaskQueue& FFRpc::getTaskQueue()
{
    return *m_tq;
}

int FFRpc::handleBroken(SocketObjPtr sock_)
{
    LOGTRACE((FFRPC, "FFRpc::handleBroken"));
    m_master_broker_sock = NULL;
    if (true == m_runing)
    {
        m_timer.onceTimer(1000, funcbind(&FFRpc::timerReconnectBroker, this));
    }
    LOGTRACE((FFRPC, "FFRpc::handleBroken end"));
    return 0;
}

int FFRpc::handleMsg(const Message& msg_, SocketObjPtr sock_)
{
    uint16_t cmd = msg_.getCmd();
    LOGTRACE((FFRPC, "FFRpc::handleMsg_impl cmd[%u] begin", cmd));

    map<uint16_t, MsgCallBack>::iterator cb = m_msgHandleFunc.find(cmd);
    if (cb != m_msgHandleFunc.end())
    {
        try
        {
            cb->second(sock_, msg_.getBody());
            LOGTRACE((FFRPC, "FFRpc::handleMsg_impl cmd[%u] call end ok", cmd));
            return 0;
        }
        catch(exception& e_)
        {
            LOGTRACE((BROKER, "FFRpc::handleMsg_impl exception<%s> body_size=%u", e_.what(), msg_.getBody().size()));
            return -1;
        }
    }
    LOGWARN((FFRPC, "FFRpc::handleMsg_impl cmd[%u] not ok", cmd));

    return -1;
}


//! 新版 调用消息对应的回调函数
int FFRpc::handleRpcCallMsg(SocketObjPtr sock_, BrokerRouteMsgReq& msg_)
{
    LOGTRACE((FFRPC, "FFRpc::handleRpcCallMsg msg begin strServiceName=%s destMsgName=%s callbackId=%u, body_size=%d",
                     msg_.destServiceName, msg_.destMsgName, msg_.callbackId, msg_.body.size()));

    if (false == msg_.errinfo.empty())
    {
        LOGERROR((FFRPC, "FFRpc::handleRpcCallMsg error=%s", msg_.errinfo));
        if (msg_.callbackId == 0)
            return 0;
    }
    if (msg_.destServiceName.empty() == false)
    {
        std::map<std::string, SharedPtr<RpcInterface> >::iterator itInterface = m_regInterface.find(msg_.destMsgName);
        if (itInterface != m_regInterface.end())
        {
            //SlotReqArg arg(msg_.body, msg_.fromNodeId, msg_.callbackId, msg_.errinfo, this);
            itInterface->second->handleMsg(msg_);
            return 0;
        }
        else
        {
            LOGERROR((FFRPC, "FFRpc::handleRpcCallMsg service=%s and msg_name=%s not found", msg_.destServiceName, msg_.destMsgName));
            msg_.errinfo = "interface named " + msg_.destMsgName + " not found in rpc";
            msg_.destNodeId = msg_.fromNodeId;
            msg_.destServiceName.clear();
            this->response("", msg_.fromNodeId, msg_.callbackId, FFThrift::EncodeAsString(msg_), msg_.errinfo);
        }
    }
    else
    {
        map<long, FFRpcCallBack>::iterator it = m_rpcTmpCallBack.find(msg_.callbackId);
        if (it != m_rpcTmpCallBack.end())
        {
            if (it->second){
                try{
                    it->second(msg_.body);
                }
                catch(exception& e_)
                {
                    LOGWARN((FFRPC, "FFRpc::callback end exception=%s", e_.what()));
                }
            }
            m_rpcTmpCallBack.erase(it);
        }
        else
        {
            LOGERROR((FFRPC, "FFRpc::handleRpcCallMsg callbackId[%u] or dest_msg=%s not found", msg_.callbackId, msg_.destMsgName));
        }
    }
    LOGTRACE((FFRPC, "FFRpc::handleRpcCallMsg msg end ok"));
    return 0;
}
//!获取某一类型的service
vector<string> FFRpc::getServicesLike(const string& token)
{
    vector<string> ret;
    map<string, int64_t>::iterator it = m_broker_data.service2nodeId.begin();
    for (; it != m_broker_data.service2nodeId.end(); ++it){
        if (it->first.find(token) != string::npos){
            ret.push_back(it->first);
        }
    }
    return ret;
}
string FFRpc::getServicesById(uint64_t destNodeId_)
{
    map<string, int64_t>::iterator it = m_broker_data.service2nodeId.begin();
    for (; it != m_broker_data.service2nodeId.end(); ++it){
        if (it->second == (int64_t)destNodeId_){
            return it->first;
        }
    }
    return "";
}
long FFRpc::docall(const string& strServiceName_, const string& msg_name_, const string& body_, FFRpcCallBack callback_)
{
    //!调用远程消息
    LOGTRACE((FFRPC, "FFRpc::docall begin strServiceName_<%s>, msg_name_<%s> body_size=%u", strServiceName_.c_str(), msg_name_.c_str(), body_.size()));

    map<string, int64_t>::iterator it = m_broker_data.service2nodeId.find(strServiceName_);
    if (it == m_broker_data.service2nodeId.end())
    {
        LOGWARN((FFRPC, "FFRpc::docall end service not exist=%s", strServiceName_));
        if (callback_){
            try{
                string err = "service " + strServiceName_ + " not exist in ffrpc";
                callback_(err);
            }
            catch(exception& e_)
            {
                LOGWARN((FFRPC, "FFRpc::docall end exception=%s", e_.what()));
            }
        }
        return -1;
    }
    static long gCallBackIDGen = 0;
    long callbackId  = ++gCallBackIDGen;
    if (callback_)
    {
        m_rpcTmpCallBack[callbackId] = callback_;
    }

    LOGTRACE((FFRPC, "FFRpc::docall end dest_id=%u callbackId=%u", it->second, callbackId));

    sendToDestNode(strServiceName_, msg_name_, it->second, callbackId, body_);

    return callbackId;
}

//! 通过node id 发送消息给broker
void FFRpc::sendToDestNode(const string& strServiceName_, const string& msg_name_,
                                uint64_t destNodeId_, int64_t callbackId_, const string& body_, string error_info)
{
    LOGTRACE((FFRPC, "FFRpc::send_to_broker_by_nodeid begin destNodeId[%u]", destNodeId_));
    BrokerRouteMsgReq dest_msg;
    dest_msg.destServiceName = strServiceName_;
    dest_msg.destMsgName = msg_name_;
    dest_msg.destNodeId = destNodeId_;
    dest_msg.callbackId = callbackId_;
    dest_msg.body = body_;
    dest_msg.errinfo = error_info;

    dest_msg.fromNodeId = m_nodeId;

    uint64_t dest_broker_id = BROKER_MASTER_nodeId;

    FFBroker* pbroker = Singleton<FFRpcMemoryRoute>::instance().getBroker(dest_broker_id);
    if (pbroker)//!如果broker和本身都在同一个进程中,那么直接内存间投递即可
    {
        LOGTRACE((FFRPC, "FFRpc::send_to_broker_by_nodeid begin destNodeId[%u], m_bind_broker_id=%u memory post", destNodeId_, dest_broker_id));
        pbroker->getTaskQueue().post(funcbind(&FFBroker::sendToRPcNode, pbroker, dest_msg));
    }
    else if (dest_broker_id == 0)
    {
        MsgSender::send(m_master_broker_sock, BROKER_ROUTE_MSG, FFThrift::EncodeAsString(dest_msg));
    }
    return;
}
//! 判断某个service是否存在
bool FFRpc::isExist(const string& strServiceName_)
{
    map<string, int64_t>::iterator it = m_broker_data.service2nodeId.begin();
    for (; it != m_broker_data.service2nodeId.end(); ++it){
        if (it->first == strServiceName_){
            return true;
        }
    }

    return true;
}


//! 调用接口后，需要回调消息给请求者
void FFRpc::response(const string& msg_name_,  uint64_t destNodeId_, int64_t callbackId_, const string& body_, string errinfo)
{
    static string null_str;
    getTaskQueue().post(funcbind(&FFRpc::sendToDestNode, this, null_str, msg_name_, destNodeId_, callbackId_, body_, errinfo));
}

//! 处理注册,
int FFRpc::handleBrokerRegResponse(SocketObjPtr sock_, RegisterToBrokerRet& msg_)
{
    LOGTRACE((FFRPC, "FFBroker::handleBrokerRegResponse begin nodeId=%d", msg_.nodeId));
    if (msg_.registerFlag < 0)
    {
        LOGERROR((FFRPC, "FFBroker::handleBrokerRegResponse registerfd failed, service exist"));
        m_master_broker_sock->close();
        m_master_broker_sock = NULL;
        return -1;
    }
    if (msg_.registerFlag == 1)
    {
        m_nodeId = msg_.nodeId;//! -1表示注册失败，0表示同步消息，1表示注册成功
        Singleton<FFRpcMemoryRoute>::instance().addNode(m_nodeId, this);
        LOGINFO((FFRPC, "FFBroker::handleBrokerRegResponse alloc nodeId=%d", m_nodeId));
    }
    m_broker_data = msg_;

    LOGTRACE((FFRPC, "FFBroker::handleBrokerRegResponse end service num=%d, ", m_broker_data.service2nodeId.size()));
    return 0;
}

//!获取broker socket
SocketObjPtr FFRpc::getBrokerSocket()
{
    return m_master_broker_sock;
}

//! 注册接口【支持动态的增加接口】
void FFRpc::reg(const string& name_, SharedPtr<RpcInterface> func_)
{
	m_regInterface[name_] = func_;
}

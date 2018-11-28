#include "server/ffworker.h"
#include "base/log.h"
#include "server/http_mgr.h"
#include "server/script_cache.h"
#include "server/db_mgr.h"

using namespace ff;
using namespace std;

#define FFWORKER_LOG                   "FFWORKER"
WorkerInitFileInfo FFWorker::gSetupFunc[100];
WorkerFunc FFWorker::gExitFunc[100];
int FFWorker::regSetupFunc(WorkerFunc f, const char* file, int line, int priority){
    static int index = -1;
    if (index == -1){
        memset(gSetupFunc, 0, sizeof(gSetupFunc));
    }
    int n = ++index;
    gSetupFunc[n].func    = f;
    gSetupFunc[n].strFile = file;
    gSetupFunc[n].nLine   = line;
    gSetupFunc[n].priority= priority;
    //printf("regSetupFunc %d %s %d\n", n, file, line);
    return 0;
}
int FFWorker::regExitFunc(WorkerFunc f){
    static int index = -1;
    if (index == -1){
        memset(gExitFunc, 0, sizeof(gExitFunc));
    }
    int n = ++index;
    gExitFunc[n] = f;
    return 0;
}
//自定义排序函数  
static bool cmpTmp(const WorkerInitFileInfo& a, const WorkerInitFileInfo& b){
    return a.priority > b.priority;//从大到小排序，从小到大排序为a<b  
}
static bool callSetupFunc(){
    std::sort(FFWorker::gSetupFunc, FFWorker::gSetupFunc+(sizeof(FFWorker::gSetupFunc) / sizeof(WorkerInitFileInfo)), cmpTmp);
    for (size_t i = 0; i < sizeof(FFWorker::gSetupFunc) / sizeof(WorkerInitFileInfo); ++i){
        if (FFWorker::gSetupFunc[i].func == NULL)
            continue;
        if ((*(FFWorker::gSetupFunc[i].func))() == false){
            LOGERROR((FFWORKER_LOG, "FFWorker::open failed when exe %s[%d]",
                                FFWorker::gSetupFunc[i].strFile, FFWorker::gSetupFunc[i].nLine));
            return false;
        }
    }
    return true;
}
static bool callExitFunc(){
    for (size_t i = 0; i < sizeof(FFWorker::gExitFunc) / sizeof(WorkerFunc); ++i){
        if (FFWorker::gExitFunc[i] == NULL)
            return true;
        (*FFWorker::gExitFunc[i])();
    }
    return true;
}
FFWorker::FFWorker():m_nWorkerIndex(0), m_allocID(0),m_ffrpc(NULL)
{
    
}
FFWorker::~FFWorker()
{
    m_shared_mem_mgr.cleanup();
}
FFWorker* FFWorker::gSingletonWorker = NULL;
int FFWorker::open(const string& brokercfg, int worker_index)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::open begin"));
    FFWorker::gSingletonWorker = this;
    
    m_nWorkerIndex = worker_index;
    char msgbuff[128] = {0};
    snprintf(msgbuff, sizeof(msgbuff), "worker#%d", worker_index);
    
    m_logic_name = msgbuff;

    m_ffrpc = new FFRpc(m_logic_name);
    m_ffrpc->reg(&FFWorker::processSessionReq, this);
    m_ffrpc->reg(&FFWorker::processSessionOffline, this);
    m_ffrpc->reg(&FFWorker::processSessionEnter, this);
    m_ffrpc->reg(&FFWorker::processWorkerCall, this);
    
    if (m_ffrpc->open(brokercfg))
    {
        LOGERROR((FFWORKER_LOG, "FFWorker::open failed check brokercfg %s", brokercfg));
        return -1;
    }
    string host = m_ffrpc->getHostCfg();
    //!tcp://127.0.0.1:43210
    vector<string> args;
    StrTool::split(host, args, ":");
    int port = ::atoi(args[2].c_str());

    m_shared_mem_mgr.init_worker(port, worker_index, m_ffrpc->getTaskQueue());
    Singleton<FFWorkerMgr>::instance().add(m_logic_name, this);
    
    LOGTRACE((FFWORKER_LOG, "FFWorker::open end ok"));
    
    SCRIPT_CACHE.init();
    DB_MGR.setDefaultTaskQueue(this->getRpc().getTaskQueue());
    EntityModule::init();
    CmdModule::init();
    return 0;
}
int FFWorker::close()
{
    if (m_ffrpc)
    {
        m_ffrpc->close();
        Singleton<FFWorkerMgr>::instance().del(m_logic_name);
    }
    m_logic_name.clear();
    return 0;
}
bool FFWorker::initModule(){
    LOGINFO((FFWORKER_LOG, "FFWorker::open initModule begin ..."));
    if (false == callSetupFunc()){
        LOGERROR((FFWORKER_LOG, "FFWorker::open initModule failed when callSetupFunc"));
        return false;
    }
    return true;
}
bool FFWorker::cleanupModule(){
    try{
        callExitFunc();
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_LOG, "cleanupModule failed er=<%s>", e_.what()));
    }
    return true;
}
//! 转发client消息
int FFWorker::processSessionReq(RPCReq<RouteLogicMsg_t::in_t, RouteLogicMsg_t::out_t>& req_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionReq begin cmd[%u]", req_.msg.cmd));
    std::map<userid_t, WorkerClient>::iterator it = m_worker_client.find(req_.msg.session_id);
    WorkerClient* pWorkerClient = NULL;
    if (it == m_worker_client.end()){
        WorkerClient& worker_client = m_worker_client[req_.msg.session_id];
        
        worker_client.from_gate = m_ffrpc->getServicesById(req_.dest_node_id);
        worker_client.session_ip= req_.msg.session_ip;
        if (worker_client.from_gate.empty())
        {
            worker_client.from_gate = "gate0";
        }
        worker_client.entity = ALLOC_ENTITY(0, 0, req_.msg.session_id);
        pWorkerClient = &worker_client;
    }
    else{
        pWorkerClient = &(it->second);
    }
    
    SessionReqEvent eMsg(pWorkerClient->entity, req_.msg.cmd, req_.msg.body);
    if (LOGOUT_CMD != req_.msg.cmd){//!如果有客户端发了错误包，忽略0xFFFF是保留协议
        EVENT_BUS_FIRE(eMsg);
    }
    if (eMsg.isDone == false){
        onSessionReq(req_.msg.session_id, req_.msg.cmd, req_.msg.body);
    }
    
    if (req_.callback_id != 0)
    {
        RouteLogicMsg_t::out_t out;
        req_.response(out);
    }
    getSharedMem().writeLockEnd();
    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionReq end ok"));
    return 0;
}

//! 处理client 下线
int FFWorker::processSessionOffline(RPCReq<SessionOffline::in_t, SessionOffline::out_t>& req_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionOffline begin"));
    EntityPtr entity = m_worker_client[req_.msg.session_id].entity;
    if (entity){
        entity->setSession(0);
    }
    SessionReqEvent eMsg(entity, LOGOUT_CMD, "", 1);
    EVENT_BUS_FIRE(eMsg);
    onSessionOffline(req_.msg.session_id);
    
    SessionOffline::out_t out;
    req_.response(out);
    m_worker_client.erase(req_.msg.session_id);
    getSharedMem().writeLockEnd();
    return 0;
}

//! 处理client 跳转
int FFWorker::processSessionEnter(RPCReq<SessionEnterWorker::in_t, SessionEnterWorker::out_t>& req_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionEnter begin gate[%s]", req_.msg.from_gate));

    WorkerClient& worker_client = m_worker_client[req_.msg.session_id];
    worker_client.from_gate = req_.msg.from_gate;
    
    SessionEnterWorker::out_t out;
    req_.response(out);
    
    
    OnSessionEnterEvent eMsg(req_.msg.session_id, req_.msg.extra_data);
    EVENT_BUS_FIRE(eMsg);
    if (eMsg.isDone == false){
        onSessionEnter(req_.msg.session_id, req_.msg.extra_data);
    }
    
    getSharedMem().writeLockEnd();
    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionEnter end ok"));

    return 0;
}

//! scene 之间的互调用
int FFWorker::processWorkerCall(RPCReq<WorkerCallMsgt::in_t, WorkerCallMsgt::out_t>& req_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::processWorkerCall begin cmd[%u]", req_.msg.cmd));
    
    WorkerCallMsgt::out_t out;
    
    OnWorkerCallEvent eMsg(req_.msg.cmd, req_.msg.body);
    EVENT_BUS_FIRE(eMsg);
    if (eMsg.isDone){
        out.body = eMsg.response;
    }
    else{
        out.body = onWorkerCall(req_.msg.cmd, req_.msg.body);
    }
    req_.response(out);

    getSharedMem().writeLockEnd();
    LOGTRACE((FFWORKER_LOG, "FFWorker::processWorkerCall end ok"));
    return 0;
}

const string& FFWorker::getSessionGate(const userid_t& session_id_)
{
    std::map<userid_t, WorkerClient>::iterator it = m_worker_client.find(session_id_);
    if (it != m_worker_client.end()){
        return it->second.from_gate;
    }
    static string nothing;
    return nothing;
}
const string& FFWorker::getSessionIp(const userid_t& session_id_)
{
    std::map<userid_t, WorkerClient>::iterator it = m_worker_client.find(session_id_);
    if (it != m_worker_client.end()){
        return it->second.session_ip;
    }
    static string nothing;
    return nothing;
}
int FFWorker::sessionSendMsg(const userid_t& session_id_, uint16_t cmd_, const string& data_)
{
    return sessionSendMsg(getSessionGate(session_id_), session_id_, cmd_, data_);
}
int FFWorker::gateBroadcastMsg(uint16_t cmd_, const string& data_)
{
    vector<string> gates = m_ffrpc->getServicesLike("gate#");
    for (size_t i = 0; i < gates.size(); ++i)
    {
        gateBroadcastMsg(gates[i], cmd_, data_);
    }
    return 0;
}
int FFWorker::sessionMulticastMsg(const vector<userid_t>& session_id_, uint16_t cmd_, const string& data_)
{
    map<string, vector<userid_t> > datas;
    for (size_t i = 0; i < session_id_.size(); ++i)
    {
        datas[getSessionGate(session_id_[i])].push_back(session_id_[i]);
    }
    map<string, vector<userid_t> >::iterator it = datas.begin();
    for (; it != datas.end(); ++it)
    {
        sessionMulticastMsg(it->first, it->second, cmd_, data_);
    }
    return 0;
}
int FFWorker::sessionClose(const userid_t& session_id_)
{
    return sessionClose(getSessionGate(session_id_), session_id_);
}
int FFWorker::sessionChangeWorker(const userid_t& session_id_, int to_worker_index_, string extra_data)
{
    char buff[128] = {0};
    snprintf(buff, sizeof(buff), "worker#%d", to_worker_index_);
    
    std::map<userid_t, WorkerClient>::iterator it = m_worker_client.find(session_id_);
    if (it != m_worker_client.end()){
        GateChangeLogicNode::in_t msg;
        msg.session_id = session_id_;
        msg.alloc_worker = buff;
        msg.extra_data = extra_data;
        m_ffrpc->call(it->second.from_gate, msg);
        m_worker_client.erase(it);
    }
    return 0;
}
//! 发送消息给特定的client
int FFWorker::sessionSendMsg(const string& gate_name, const userid_t& session_id_, uint16_t cmd_, const string& data_)
{
    if (gate_name.empty())
        return -1;
    LOGTRACE((FFWORKER_LOG, "FFWorker::send_msg_session begin session_id_<%ld>", session_id_));

    GateRouteMsgToSession::in_t msg;
    msg.session_id.push_back(session_id_);
    msg.cmd  = cmd_;
    msg.body = data_;
    m_ffrpc->call(gate_name, msg);
    LOGTRACE((FFWORKER_LOG, "FFWorker::send_msg_session end ok gate[%s]", gate_name));
    return 0;
}
int FFWorker::sessionMulticastMsg(const string& gate_name, const vector<userid_t>& session_id_, uint16_t cmd_, const string& data_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::multicast_msg_session begin session_id_<%u>", session_id_.size()));

    GateRouteMsgToSession::in_t msg;
    msg.session_id = session_id_;
    msg.cmd  = cmd_;
    msg.body = data_;
    m_ffrpc->call(gate_name, msg);
    LOGTRACE((FFWORKER_LOG, "FFWorker::multicast_msg_session end ok gate[%s]", gate_name));
    return 0;
}
int FFWorker::sessionKFSendMsg(const string& group_name, const string& gate_name,
                                   const userid_t& session_id_,
                                   uint16_t cmd_, const string& data_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::send_msg_session begin session_id_<%ld>", session_id_));

    GateRouteMsgToSession::in_t msg;
    msg.session_id.push_back(session_id_);
    msg.cmd  = cmd_;
    msg.body = data_;
    m_ffrpc->call(group_name, gate_name, msg);
    LOGTRACE((FFWORKER_LOG, "FFWorker::send_msg_session end ok gate[%s]", gate_name));
    return 0;
}
//! 广播 整个gate
int FFWorker::gateBroadcastMsg(const string& gate_name_, uint16_t cmd_, const string& data_)
{
    GateBroadcastMsgToSession::in_t msg;
    msg.cmd = cmd_;
    msg.body = data_;
    m_ffrpc->call(gate_name_, msg);
    return 0;
}
//! 关闭某个session
int FFWorker::sessionClose(const string& gate_name_, const userid_t& session_id_)
{
    GateCloseSession::in_t msg;
    msg.session_id = session_id_;
    m_ffrpc->call(gate_name_, msg);
    return 0;
}
struct timer_cb
{
    static void callback(FFWorker* w, Task func)
    {
        func.run();
        w->getSharedMem().writeLockEnd();
    }
};
void FFWorker::regTimer(uint64_t mstimeout_, Task func){
    getRpc().getTimer().onceTimer(mstimeout_, TaskBinder::gen(&timer_cb::callback, this, func));
}
void FFWorker::workerRPC(int workerindex, uint16_t cmd, const std::string& data, FFSlot::FFCallBack* cb){
    WorkerCallMsgt::in_t reqmsg;
    reqmsg.cmd = cmd;
    reqmsg.body= data;
    char service_name[128] = {0};
    snprintf(service_name, sizeof(service_name), "worker#%d", workerindex);
    
    getRpc().call(service_name, reqmsg, cb);
}
void FFWorker::asyncHttp(const std::string& url_, int timeoutsec, FFSlot::FFCallBack* cb){
    Singleton<HttpMgr>::instance().request(url_, timeoutsec, cb);
}
std::string FFWorker::syncHttp(const std::string& url_, int timeoutsec){
    return Singleton<HttpMgr>::instance().syncRequest(url_, timeoutsec);
}

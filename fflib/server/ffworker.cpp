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
    LOGTRACE((FFWORKER_LOG, "FFWorker::~FFWorker begin"));
    LOGTRACE((FFWORKER_LOG, "FFWorker::~FFWorker begin2"));
    m_ffrpc = NULL;
    LOGTRACE((FFWORKER_LOG, "FFWorker::~FFWorker end"));
}

struct ScriptTimerTool{
    static void doTimer(ScriptArgObjPtr funcOBj){
        if (funcOBj && funcOBj->isFunc()){
            funcOBj->runFunc(ScriptArgObj::create());
        }
    }
};
void FFWorker::regTimerForScirpt(uint64_t mstimeout_, ScriptArgObjPtr func){
    regTimer(mstimeout_, funcbind(&ScriptTimerTool::doTimer, func));
}
void FFWorker::logdebugForScirpt(const std::string& content_) {LOGDEBUG((FFWORKER_LOG, "%s", content_));}
void FFWorker::logtraceForScirpt(const std::string& content_) {LOGTRACE((FFWORKER_LOG, "%s", content_));}
void FFWorker::loginfoForScirpt (const std::string& content_) { LOGINFO((FFWORKER_LOG, "%s", content_));}
void FFWorker::logwarnForScirpt (const std::string& content_) { LOGWARN((FFWORKER_LOG, "%s", content_));}
void FFWorker::logerrorForScirpt(const std::string& content_) {LOGERROR((FFWORKER_LOG, "%s", content_));}
void FFWorker::logfatalForScirpt(const std::string& content_) {LOGFATAL((FFWORKER_LOG, "%s", content_));}

static ScriptArgObjPtr queryResultToScriptArg(QueryDBResult& result)
{
    map<string, ScriptArgObjPtr> retval;
    retval["datas"] = ScriptArgUtil<vector<vector<string> > >::toValue(result.dataResult);
    retval["fieldNames"] = ScriptArgUtil<vector<string> >::toValue(result.fieldNames);
    retval["errinfo"] = ScriptArgUtil<string>::toValue(result.errinfo);
    retval["affectedRows"] = ScriptArgUtil<int>::toValue(result.affectedRows);

    ScriptArgObjPtr funcArg = ScriptArgObj::create();
    funcArg->toDict(&retval);
    LOGTRACE((FFWORKER_LOG, "FFWorker::datas=%d", result.dataResult.size()));
    return funcArg;
}

static void asyncQueryCB(ScriptArgObjPtr funcOBj, QueryDBResult& result){
    if (funcOBj && funcOBj->isFunc()){
        funcOBj->runFunc(queryResultToScriptArg(result));
    }
}
void FFWorker::asyncQuery(int64_t modid, const std::string& sql_, ScriptArgObjPtr func)
{
    DbMgr::instance().asyncQuery(modid, sql_, funcbind(&asyncQueryCB, func), &(getRpc().getTaskQueue()));
}
void FFWorker::asyncQueryByCfg(const string& cfg, int64_t modid, const string& sql_, ScriptArgObjPtr func)
{
    DbMgr::instance().asyncQueryByCfg(cfg, modid, sql_, funcbind(&asyncQueryCB, func), &(getRpc().getTaskQueue()));
}
ScriptArgObjPtr FFWorker::query(const string& sql_)
{
    QueryDBResult result;
    DbMgr::instance().query(sql_, result);
    return queryResultToScriptArg(result);
}
ScriptArgObjPtr FFWorker::queryByCfg(const string& cfg, const string& sql_)
{
    QueryDBResult result;
    DbMgr::instance().queryByCfg(cfg, sql_, result);
    return queryResultToScriptArg(result);
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
        m_ffrpc->close();
        return -1;
    }
    string host = m_ffrpc->getHostCfg();
    //!tcp://127.0.0.1:43210
    vector<string> args;
    StrTool::split(host, args, ":");
    //int port = ::atoi(args[2].c_str());

    Singleton<FFWorkerMgr>::instance().add(m_logic_name, this);

    LOGTRACE((FFWORKER_LOG, "FFWorker::open end ok"));

    SCRIPT_CACHE.init();
    EntityModule::init();
    CmdModule::init();

    SCRIPT_UTIL.reg("escape", &FFDb::escape);
    SCRIPT_UTIL.reg("sessionSendMsg", &FFWorker::sessionSendMsg, this);
    SCRIPT_UTIL.reg("gateBroadcastMsg", &FFWorker::gateBroadcastMsg, this);
    SCRIPT_UTIL.reg("sessionMulticastMsg", &FFWorker::sessionMulticastMsg, this);
    SCRIPT_UTIL.reg("sessionClose", &FFWorker::sessionClose, this);
    SCRIPT_UTIL.reg("sessionChangeWorker", &FFWorker::sessionChangeWorker, this);
    SCRIPT_UTIL.reg("getSessionGate", &FFWorker::getSessionGate, this);
    SCRIPT_UTIL.reg("getSessionIp", &FFWorker::getSessionIp, this);
    SCRIPT_UTIL.reg("isExist", &FFRpc::isExist, (FFRpc*)(&this->getRpc()));
    //SCRIPT_UTIL.reg("reload", &FFWorker::reload, this);

    SCRIPT_UTIL.reg("logdebug", &FFWorker::logdebugForScirpt, this);
    SCRIPT_UTIL.reg("logtrace", &FFWorker::logtraceForScirpt, this);
    SCRIPT_UTIL.reg("loginfo",  &FFWorker::loginfoForScirpt , this);
    SCRIPT_UTIL.reg("logwarn",  &FFWorker::logwarnForScirpt , this);
    SCRIPT_UTIL.reg("logerror", &FFWorker::logerrorForScirpt, this);
    SCRIPT_UTIL.reg("logfatal", &FFWorker::logfatalForScirpt, this);

    SCRIPT_UTIL.reg("regTimer", &FFWorker::regTimerForScirpt, this);
    SCRIPT_UTIL.reg("initDBPool", &DbMgr::initDBPool, (DbMgr*)(&(DbMgr::instance())));
    SCRIPT_UTIL.reg("asyncQuery", &FFWorker::asyncQuery, this);
    SCRIPT_UTIL.reg("query", &FFWorker::query, this);
    SCRIPT_UTIL.reg("asyncQueryByCfg", &FFWorker::asyncQueryByCfg, this);
    SCRIPT_UTIL.reg("queryByCfg", &FFWorker::queryByCfg, this);
    SCRIPT_UTIL.reg("workerRPC", &FFWorker::workerRPC, this);
    SCRIPT_UTIL.reg("asyncHttp", &FFWorker::asyncHttp, this);
    SCRIPT_UTIL.reg("syncHttp", &FFWorker::syncHttp, this);
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
int FFWorker::processSessionReq(RPCReq<RouteLogicMsgReq, EmptyMsgRet>& req_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionReq begin cmd[%u]", req_.msg.cmd));
    std::map<userid_t, WorkerClient>::iterator it = m_worker_client.find(req_.msg.sessionId);
    WorkerClient* pWorkerClient = NULL;
    if (it == m_worker_client.end()){
        WorkerClient& worker_client = m_worker_client[req_.msg.sessionId];

        worker_client.fromGate = m_ffrpc->getServicesById(req_.destNodeId);
        worker_client.sessionIp= req_.msg.sessionIp;
        if (worker_client.fromGate.empty())
        {
            worker_client.fromGate = "gate0";
        }
        worker_client.entity = ALLOC_ENTITY(0, 0, req_.msg.sessionId);
        pWorkerClient = &worker_client;
    }
    else{
        pWorkerClient = &(it->second);
    }

    SessionReqEvent eMsg(pWorkerClient->entity, req_.msg.cmd, req_.msg.body);
    if (LOGOUT_CMD != (int)req_.msg.cmd){//!如果有客户端发了错误包，忽略0xFFFF是保留协议
        EVENT_BUS_FIRE(eMsg);
    }
    if (eMsg.isDone == false){
        onSessionReq(req_.msg.sessionId, req_.msg.cmd, req_.msg.body);
    }

    if (req_.callbackId != 0)
    {
        EmptyMsgRet out;
        req_.response(out);
    }

    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionReq end ok"));
    return 0;
}

//! 处理client 下线
int FFWorker::processSessionOffline(RPCReq<SessionOfflineReq, EmptyMsgRet>& req_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionOffline begin"));
    EntityPtr entity = m_worker_client[req_.msg.sessionId].entity;
    if (entity){
        entity->setSession(0);
    }
    SessionReqEvent eMsg(entity, LOGOUT_CMD, "", 1);
    EVENT_BUS_FIRE(eMsg);
    onSessionOffline(req_.msg.sessionId);

    EmptyMsgRet out;
    req_.response(out);
    m_worker_client.erase(req_.msg.sessionId);

    return 0;
}

//! 处理client 跳转
int FFWorker::processSessionEnter(RPCReq<SessionEnterWorkerReq, EmptyMsgRet>& req_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionEnter begin gate[%s]", req_.msg.fromGate));

    WorkerClient& worker_client = m_worker_client[req_.msg.sessionId];
    worker_client.fromGate = req_.msg.fromGate;

    EmptyMsgRet out;
    req_.response(out);


    OnSessionEnterEvent eMsg(req_.msg.sessionId, req_.msg.extraData);
    EVENT_BUS_FIRE(eMsg);
    if (eMsg.isDone == false){
        onSessionEnter(req_.msg.sessionId, req_.msg.extraData);
    }


    LOGTRACE((FFWORKER_LOG, "FFWorker::processSessionEnter end ok"));

    return 0;
}

//! scene 之间的互调用
int FFWorker::processWorkerCall(RPCReq<WorkerCallMsgReq, WorkerCallMsgRet>& req_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::processWorkerCall begin cmd[%u]", req_.msg.cmd));

    WorkerCallMsgRet out;

    OnWorkerCallEvent eMsg(req_.msg.cmd, req_.msg.body);
    EVENT_BUS_FIRE(eMsg);
    if (eMsg.isDone){
        out.body = eMsg.response;
    }
    else{
        out.body = onWorkerCall(req_.msg.cmd, req_.msg.body);
    }
    req_.response(out);


    LOGTRACE((FFWORKER_LOG, "FFWorker::processWorkerCall end ok"));
    return 0;
}

string FFWorker::getSessionGate(const userid_t& sessionId_)
{
    std::map<userid_t, WorkerClient>::iterator it = m_worker_client.find(sessionId_);
    if (it != m_worker_client.end()){
        return it->second.fromGate;
    }
    return "";
}
string FFWorker::getSessionIp(const userid_t& sessionId_)
{
    std::map<userid_t, WorkerClient>::iterator it = m_worker_client.find(sessionId_);
    if (it != m_worker_client.end()){
        return it->second.sessionIp;
    }
    return "";
}
int FFWorker::sessionSendMsg(const userid_t& sessionId_, uint16_t cmd_, const string& data_)
{
    return sessionSendMsgToGate(getSessionGate(sessionId_), sessionId_, cmd_, data_);
}
int FFWorker::gateBroadcastMsg(uint16_t cmd_, const string& data_)
{
    vector<string> gates = m_ffrpc->getServicesLike("gate#");
    for (size_t i = 0; i < gates.size(); ++i)
    {
        gateBroadcastMsgToGate(gates[i], cmd_, data_);
    }
    return 0;
}
int FFWorker::sessionMulticastMsg(const vector<userid_t>& sessionId_, uint16_t cmd_, const string& data_)
{
    map<string, vector<userid_t> > datas;
    for (size_t i = 0; i < sessionId_.size(); ++i)
    {
        datas[getSessionGate(sessionId_[i])].push_back(sessionId_[i]);
    }
    map<string, vector<userid_t> >::iterator it = datas.begin();
    for (; it != datas.end(); ++it)
    {
        sessionMulticastMsgToGate(it->first, it->second, cmd_, data_);
    }
    return 0;
}
int FFWorker::sessionClose(const userid_t& sessionId_)
{
    return sessionCloseToGate(getSessionGate(sessionId_), sessionId_);
}
int FFWorker::sessionChangeWorker(const userid_t& sessionId_, int toWorker_index_, string extraData)
{
    char buff[128] = {0};
    snprintf(buff, sizeof(buff), "worker#%d", toWorker_index_);

    std::map<userid_t, WorkerClient>::iterator it = m_worker_client.find(sessionId_);
    if (it != m_worker_client.end()){
        GateChangeLogicNodeReq msg;
        msg.sessionId = sessionId_;
        msg.allocWorker = buff;
        msg.extraData = extraData;
        m_ffrpc->asyncCall(it->second.fromGate, msg);
        m_worker_client.erase(it);
    }
    return 0;
}
//! 发送消息给特定的client
int FFWorker::sessionSendMsgToGate(const string& gate_name, const userid_t& sessionId_, uint16_t cmd_, const string& data_)
{
    if (gate_name.empty())
        return -1;
    LOGTRACE((FFWORKER_LOG, "FFWorker::send_msg_session begin sessionId_<%ld>", sessionId_));

    GateRouteMsgToSessionReq msg;
    msg.sessionId.push_back(sessionId_);
    msg.cmd  = cmd_;
    msg.body = data_;
    m_ffrpc->asyncCall(gate_name, msg);
    LOGTRACE((FFWORKER_LOG, "FFWorker::send_msg_session end ok gate[%s]", gate_name));
    return 0;
}
int FFWorker::sessionMulticastMsgToGate(const string& gate_name, const vector<userid_t>& sessionId_, uint16_t cmd_, const string& data_)
{
    LOGTRACE((FFWORKER_LOG, "FFWorker::multicast_msg_session begin sessionId_<%u>", sessionId_.size()));

    GateRouteMsgToSessionReq msg;
    msg.sessionId = sessionId_;
    msg.cmd  = cmd_;
    msg.body = data_;
    m_ffrpc->asyncCall(gate_name, msg);
    LOGTRACE((FFWORKER_LOG, "FFWorker::multicast_msg_session end ok gate[%s]", gate_name));
    return 0;
}

//! 广播 整个gate
int FFWorker::gateBroadcastMsgToGate(const string& gate_name_, uint16_t cmd_, const string& data_)
{
    GateBroadcastMsgToSessionReq msg;
    msg.cmd = cmd_;
    msg.body = data_;
    LOGTRACE((FFWORKER_LOG, "FFWorker::gateBroadcastMsgToGate begin[%s]", gate_name_));
    //m_ffrpc->call(gate_name_, msg);
    m_ffrpc->asyncCall(gate_name_, msg);
    return 0;
}
//! 关闭某个session
int FFWorker::sessionCloseToGate(const string& gate_name_, const userid_t& sessionId_)
{
    GateCloseSessionReq msg;
    msg.sessionId = sessionId_;
    m_ffrpc->asyncCall(gate_name_, msg);
    return 0;
}

void FFWorker::regTimer(uint64_t mstimeout_, Function<void()> func){
    getRpc().getTimer().onceTimer(mstimeout_, func);
}
static void RpcCallBack(ScriptArgObjPtr funcOBj, const string& data){
    if (funcOBj && funcOBj->isFunc()){

        WorkerCallMsgRet retmsg;
        try{
            FFThrift::DecodeFromString(retmsg, data);
        }
        catch(exception& e_)
        {
        }
        funcOBj->runFunc(ScriptArgUtil<string>::toValue(retmsg.body));
    }
}
void FFWorker::workerRPC(int workerindex, uint16_t cmd, const std::string& data, ScriptArgObjPtr func){
    WorkerCallMsgReq reqmsg;
    reqmsg.cmd = cmd;
    reqmsg.body= data;
    char strServiceName[128] = {0};
    snprintf(strServiceName, sizeof(strServiceName), "worker#%d", workerindex);

    getRpc().asyncCall(strServiceName, reqmsg, funcbind(&RpcCallBack, func));
}
static void HttpCallBack(ScriptArgObjPtr funcOBj, const string& data){
    if (funcOBj && funcOBj->isFunc()){
        funcOBj->runFunc(ScriptArgUtil<string>::toValue(data));
    }
}
void FFWorker::asyncHttp(const std::string& url_, int timeoutsec, ScriptArgObjPtr func){
    Singleton<HttpMgr>::instance().request(url_, timeoutsec, funcbind(&HttpCallBack, func));
}
std::string FFWorker::syncHttp(const std::string& url_, int timeoutsec){
    return Singleton<HttpMgr>::instance().syncRequest(url_, timeoutsec);
}

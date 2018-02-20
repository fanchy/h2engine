
//脚本

#include "./ffworker_lua.h"
#include "base/performance_daemon.h"
#include "server/http_mgr.h"
#include "server/script.h"

#include <stdio.h> 
#include "luaops.h"
using namespace ff;
using namespace std;


FFWorkerLua::FFWorkerLua():
    m_enable_call(true), m_started(false)
{
    m_fflua = new luaops_t();
}
FFWorkerLua::~FFWorkerLua()
{
    delete m_fflua;
    m_fflua = NULL;
}
static bool  lua_send_msg_session(userid_t session_id_, uint16_t cmd_, const string& data_)
{
    Singleton<FFWorkerLua>::instance().sessionSendMsg(session_id_, cmd_, data_);
    return true;
}
static bool  lua_broadcast_msg_gate(uint16_t cmd_, const string& data_)
{
    Singleton<FFWorkerLua>::instance().gateBroadcastMsg(cmd_, data_);
    return true;
}
static bool  lua_multicast_msg_session(const vector<userid_t>& session_id_, uint16_t cmd_, const string& data_)
{
    Singleton<FFWorkerLua>::instance().sessionMulticastMsg(session_id_, cmd_, data_);
    return true;
}
static bool  lua_closeSession(userid_t session_id_)
{
    Singleton<FFWorkerLua>::instance().sessionClose(session_id_);
    return true;
}
static bool  lua_change_session_worker(userid_t session_id_, int to_worker_index_, string extra_data)
{
    Singleton<FFWorkerLua>::instance().sessionChangeWorker(session_id_, to_worker_index_, extra_data);
    return true;
}
static string lua_getSessionGate(userid_t session_id_)
{
    return Singleton<FFWorkerLua>::instance().getSessionGate(session_id_);
}
static string lua_getSessionIp(userid_t session_id_)
{
    return Singleton<FFWorkerLua>::instance().getSessionIp(session_id_);
}
//! 判断某个service是否存在
static bool lua_isExist(const string& service_name_)
{
    return Singleton<FFWorkerLua>::instance().getRpc().isExist(service_name_);
}
static string lua_reload(const string& name_)
{
    return Singleton<FFWorkerLua>::instance().reload(name_);
}
static bool  lua_log(int level, const string& mod_, const string& content_)
{
    Singleton<FFWorkerLua>::instance().pylog(level, mod_, content_);
    return true;
}

static int  lua_regTimer(lua_State* ls_)
{
    static int64_t timer_idx = 0;
    int mstimeout_ = 0;
    luacpp_op_t<int>::lua2cpp(ls_, ADDR_ARG_POS(1), mstimeout_);
    
    char fieldname[256] = {0};
    ++timer_idx;
    long idx = long(timer_idx);
    snprintf(fieldname, sizeof(fieldname), "timer#%ld", idx);
    
    lua_getglobal(ls_, EXT_NAME);
    lua_pushstring(ls_, fieldname);
    lua_pushvalue(ls_, ADDR_ARG_POS(2));
    lua_settable(ls_, -3);
    lua_pop(ls_, 1);
    
    struct lambda_cb
    {
        static void callback(lua_State* ls_, long idx)
        {
            char fieldname[256] = {0};
            snprintf(fieldname, sizeof(fieldname), "timer#%ld", idx);
            
            lua_getglobal(ls_, EXT_NAME);
            lua_pushstring(ls_, fieldname);
            lua_gettable (ls_, -2);
            try
            {
                if (::lua_pcall(ls_, 0, 0, 0) != 0)
                {
                    string err = lua_err_handler_t::luatraceback(ls_, "lua_pcall faled func_name<%s>", fieldname);
                    ::lua_pop(ls_, 1);
                    throw lua_err_t(err);
                }
                //Singleton<FFWorkerLua>::instance().getFFlua().call_lambda<void>(pFunc);
            }
            catch(exception& e_)
            {
                LOGERROR((FFWORKER_LUA, "FFWorkerLua::lua_regTimer exception<%s>", e_.what()));
            }
            
            lua_pushstring(ls_, fieldname);
            lua_pushnil(ls_);
            lua_settable(ls_, -3);
            lua_pop(ls_, 1);
        }
    };
    
    Singleton<FFWorkerLua>::instance().regTimer(mstimeout_, 
                TaskBinder::gen(&lambda_cb::callback, ls_, idx));
    return 0;
}
static bool  lua_writeLockGuard(){
    Singleton<FFWorkerLua>::instance().getSharedMem().writeLockGuard();
    return true;
}
//!数据库相关操作
static long lua_connectDB(const string& host_, const string& group_)
{
    return DB_MGR.connectDB(host_, group_);
}
struct AsyncQueryCB
{
    AsyncQueryCB(lua_State* lsaarg_, long idxarg):ls_(lsaarg_), idx(idxarg){}
    void operator()(DbMgr::queryDBResult_t& result)
    {
        DbMgr::queryDBResult_t* data = &result;
        call_lua(ls_, idx, data->errinfo, data->dataResult, data->fieldNames, data->affectedRows);
    }
    void call_lua(lua_State* ls_, long idx, string errinfo, vector<vector<string> > ret_, vector<string> col_, int affectedRows)
    {
        if (Singleton<FFWorkerLua>::instance().m_enable_call == false)
        {
            return;
        }
        char fieldname[256] = {0};
        snprintf(fieldname, sizeof(fieldname), "db#%ld", idx);
        
        lua_getglobal(ls_, EXT_NAME);
        lua_pushstring(ls_, fieldname);
        lua_gettable (ls_, -2);
        
        
        lua_newtable(ls_);
        {
            string key = "datas";
            luacpp_op_t<string>::cpp2luastack(ls_, key);
            luacpp_op_t<vector<vector<string> > >::cpp2luastack(ls_, ret_);
            lua_settable(ls_, -3);
        }
        
        {
            string key = "fields";
            luacpp_op_t<string>::cpp2luastack(ls_, key);
            luacpp_op_t<vector<string> >::cpp2luastack(ls_, col_);
            lua_settable(ls_, -3);
        }
        {
            string key = "errinfo";
            luacpp_op_t<string>::cpp2luastack(ls_, key);
            luacpp_op_t<string >::cpp2luastack(ls_, errinfo);
            lua_settable(ls_, -3);
        }
        {
            string key = "affectedRows";
            
            luacpp_op_t<string>::cpp2luastack(ls_, key);
            luacpp_op_t<int >::cpp2luastack(ls_, affectedRows);
            lua_settable(ls_, -3);
        }
        try
        {
            if (::lua_pcall(ls_, 1, 0, 0) != 0)
            {
                string err = lua_err_handler_t::luatraceback(ls_, "lua_pcall faled func_name<%s>", fieldname);
                ::lua_pop(ls_, 1);
                throw lua_err_t(err);
            }
            //Singleton<FFWorkerLua>::instance().getFFlua().call_lambda<void>(pFunc);
        }
        catch(exception& e_)
        {
            LOGERROR((FFWORKER_LUA, "workerobj_lua_t::gen_queryDB_callback exception<%s>", e_.what()));
        }
        
        lua_pushstring(ls_, fieldname);
        lua_pushnil(ls_);
        lua_settable(ls_, -3);
        lua_pop(ls_, 1);
    }
    lua_State* ls_;
    long idx;
};

static int64_t db_idx = 0;
static int lua_asyncQuery(lua_State* ls_)
{
    long db_id_ = 0;
    luacpp_op_t<long>::lua2cpp(ls_, ADDR_ARG_POS(1), db_id_);
    
    string sql_;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(2), sql_);
    
    char fieldname[256] = {0};
    ++db_idx;
    long idx = long(db_idx);
    snprintf(fieldname, sizeof(fieldname), "db#%ld", idx);
    
    lua_getglobal(ls_, EXT_NAME);
    lua_pushstring(ls_, fieldname);
    lua_pushvalue(ls_, ADDR_ARG_POS(3));
    lua_settable(ls_, -3);
    lua_pop(ls_, 1);
    
    AsyncQueryCB cb(ls_, idx);
    DB_MGR.asyncQueryModId(db_id_, sql_, cb, &(Singleton<FFWorkerLua>::instance().getRpc().get_tq()));
    return 0;
}
struct AsyncQueryNameCB
{
    AsyncQueryNameCB(lua_State* lsaarg_, long idxarg):ls_(lsaarg_), idx(idxarg){}
    void operator()(DbMgr::queryDBResult_t& result)
    {
        DbMgr::queryDBResult_t* data = (DbMgr::queryDBResult_t*)(&result);
        call_lua(ls_, idx, data->errinfo, data->dataResult, data->fieldNames, data->affectedRows);
    }
    void call_lua(lua_State* ls_, long idx, string errinfo, vector<vector<string> > ret_, vector<string> col_, int affectedRows)
    {
        if (Singleton<FFWorkerLua>::instance().m_enable_call == false)
        {
            return;
        }
        char fieldname[256] = {0};
        snprintf(fieldname, sizeof(fieldname), "db#%ld", idx);
        
        lua_getglobal(ls_, EXT_NAME);
        lua_pushstring(ls_, fieldname);
        lua_gettable (ls_, -2);
        
        
        lua_newtable(ls_);
        {
            string key = "datas";
            luacpp_op_t<string>::cpp2luastack(ls_, key);
            luacpp_op_t<vector<vector<string> > >::cpp2luastack(ls_, ret_);
            lua_settable(ls_, -3);
        }
        
        {
            string key = "fields";
            luacpp_op_t<string>::cpp2luastack(ls_, key);
            luacpp_op_t<vector<string> >::cpp2luastack(ls_, col_);
            lua_settable(ls_, -3);
        }
        {
            string key = "errinfo";
            luacpp_op_t<string>::cpp2luastack(ls_, key);
            luacpp_op_t<string >::cpp2luastack(ls_, errinfo);
            lua_settable(ls_, -3);
        }
        {
            string key = "affectedRows";
            
            luacpp_op_t<string>::cpp2luastack(ls_, key);
            luacpp_op_t<int >::cpp2luastack(ls_, affectedRows);
            lua_settable(ls_, -3);
        }
        try
        {
            if (::lua_pcall(ls_, 1, 0, 0) != 0)
            {
                string err = lua_err_handler_t::luatraceback(ls_, "lua_pcall faled func_name<%s>", fieldname);
                ::lua_pop(ls_, 1);
                throw lua_err_t(err);
            }
            //Singleton<FFWorkerLua>::instance().getFFlua().call_lambda<void>(pFunc);
        }
        catch(exception& e_)
        {
            LOGERROR((FFWORKER_LUA, "workerobj_lua_t::gen_queryDB_callback exception<%s>", e_.what()));
        }
        
        lua_pushstring(ls_, fieldname);
        lua_pushnil(ls_);
        lua_settable(ls_, -3);
        lua_pop(ls_, 1);
    }
    lua_State* ls_;
    long idx;
};

static int lua_asyncQueryByName(lua_State* ls_)
{
    string name;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(1), name);
    
    //long mod_ = 0;
    //luacpp_op_t<long>::lua2cpp(ls_, ADDR_ARG_POS(2), mod_);
    
    string sql_;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(2), sql_);
    
    char fieldname[256] = {0};
    ++db_idx;
    long idx = long(db_idx);
    snprintf(fieldname, sizeof(fieldname), "db#%ld", idx);
    
    lua_getglobal(ls_, EXT_NAME);
    lua_pushstring(ls_, fieldname);
    lua_pushvalue(ls_, ADDR_ARG_POS(3));
    lua_settable(ls_, -3);
    lua_pop(ls_, 1);
    
    AsyncQueryNameCB cb(ls_, idx);
    DB_MGR.asyncQueryByName(name, sql_, cb, &(Singleton<FFWorkerLua>::instance().getRpc().get_tq()));
    return 0;
}
static int lua_queryByName(lua_State* ls_)
{
    string name;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(1), name);
    
    //long mod_ = 0;
    //luacpp_op_t<long>::lua2cpp(ls_, ADDR_ARG_POS(2), mod_);
    
    string sql_;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(2), sql_);
    
    string errinfo;
    vector<vector<string> > retdata;
    vector<string> col;
    int affectedRows = 0;
    DB_MGR.queryByName(name, sql_, &retdata, &errinfo, &affectedRows, &col);
    
    lua_newtable(ls_);
    {
        string key = "datas";
        luacpp_op_t<string>::cpp2luastack(ls_, key);
        luacpp_op_t<vector<vector<string> > >::cpp2luastack(ls_, retdata);
        lua_settable(ls_, -3);
    }
    
    {
        string key = "fields";
        luacpp_op_t<string>::cpp2luastack(ls_, key);
        luacpp_op_t<vector<string> >::cpp2luastack(ls_, col);
        lua_settable(ls_, -3);
    }
    {
        string key = "errinfo";
        luacpp_op_t<string>::cpp2luastack(ls_, key);
        luacpp_op_t<string >::cpp2luastack(ls_, errinfo);
        lua_settable(ls_, -3);
    }
    {
        string key = "affectedRows";
        
        luacpp_op_t<string>::cpp2luastack(ls_, key);
        luacpp_op_t<int >::cpp2luastack(ls_, affectedRows);
        lua_settable(ls_, -3);
    }
    return 1;
}

static int lua_query(lua_State* ls_)
{
    //long db_id_ = 0;
    //luacpp_op_t<long>::lua2cpp(ls_, ADDR_ARG_POS(1), db_id_);
    
    string sql_;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(1), sql_);
    
    string errinfo;
    vector<vector<string> > retdata;
    vector<string> col;
    int affectedRows = 0;
    DB_MGR.query(sql_, &retdata, &errinfo, &affectedRows, &col);
    
    lua_newtable(ls_);
    {
        string key = "datas";
        luacpp_op_t<string>::cpp2luastack(ls_, key);
        luacpp_op_t<vector<vector<string> > >::cpp2luastack(ls_, retdata);
        lua_settable(ls_, -3);
    }
    
    {
        string key = "fields";
        luacpp_op_t<string>::cpp2luastack(ls_, key);
        luacpp_op_t<vector<string> >::cpp2luastack(ls_, col);
        lua_settable(ls_, -3);
    }
    {
        string key = "errinfo";
        luacpp_op_t<string>::cpp2luastack(ls_, key);
        luacpp_op_t<string >::cpp2luastack(ls_, errinfo);
        lua_settable(ls_, -3);
    }
    {
        string key = "affectedRows";
        
        luacpp_op_t<string>::cpp2luastack(ls_, key);
        luacpp_op_t<int >::cpp2luastack(ls_, affectedRows);
        lua_settable(ls_, -3);
    }
    return 1;
}
//!调用其他worker的接口 
static int lua_workerRPC(lua_State* ls_){
    //int workerindex, int16_t cmd, const string& argdata, PyObject* pFuncArg
    
    int workerindex = 0;
    luacpp_op_t<int>::lua2cpp(ls_, ADDR_ARG_POS(1), workerindex);
    
    uint16_t cmd = 0;
    luacpp_op_t<uint16_t>::lua2cpp(ls_, ADDR_ARG_POS(2), cmd);
    
    string argdata;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(3), argdata);
    
    static int64_t rpc_idx = 0;
    char fieldname[256] = {0};
    ++rpc_idx;
    long idx = long(rpc_idx);
    snprintf(fieldname, sizeof(fieldname), "rpc#%ld", idx);
    
    lua_getglobal(ls_, EXT_NAME);
    lua_pushstring(ls_, fieldname);
    lua_pushvalue(ls_, ADDR_ARG_POS(4));
    lua_settable(ls_, -3);
    lua_pop(ls_, 1);
    
    struct lambda_cb: public FFSlot::FFCallBack
    {
        lambda_cb(lua_State* lsarg, int idxarg):ls_(lsarg), idx(idxarg){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(ffslot_req_arg))
            {
                return;
            }
            ffslot_req_arg* msg_data = (ffslot_req_arg*)args_;
            WorkerCallMsgt::out_t retmsg;
            try{
                FFThrift::DecodeFromString(retmsg, msg_data->body);
            }
            catch(exception& e_)
            {
            }
            
            char fieldname[256] = {0};
            snprintf(fieldname, sizeof(fieldname), "rpc#%ld", idx);
            
            lua_getglobal(ls_, EXT_NAME);
            lua_pushstring(ls_, fieldname);
            lua_gettable (ls_, -2);
            
            lua_pushlstring(ls_, retmsg.body.c_str(), retmsg.body.size());
            try
            {
                if (::lua_pcall(ls_, 1, 0, 0) != 0)
                {
                    string err = lua_err_handler_t::luatraceback(ls_, "lua_pcall faled func_name<%s>", fieldname);
                    ::lua_pop(ls_, 1);
                    throw lua_err_t(err);
                }
            }
            catch(exception& e_)
            {
                LOGERROR((FFWORKER_LUA, "FFWorkerLua::call_service exception=%s", e_.what()));
            }
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(ls_, idx); }
        lua_State* ls_;
        long idx;
    };
    Singleton<FFWorkerLua>::instance().workerRPC(workerindex, cmd, argdata, new lambda_cb(ls_, idx));
    return 0;
}

static bool  lua_syncSharedData(int cmd, const string& data)
{
    Singleton<FFWorkerLua>::instance().getSharedMem().syncSharedData(cmd, data);
    return true;
}

static int lua_asyncHttp(lua_State* ls_)
{
    string url;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(1), url);
    
    int timeoutsec = 0;
    luacpp_op_t<int>::lua2cpp(ls_, ADDR_ARG_POS(2), timeoutsec);
    
    static int64_t http_idx = 0;
    char fieldname[256] = {0};
    ++http_idx;
    long idx = long(http_idx);
    snprintf(fieldname, sizeof(fieldname), "http#%ld", idx);
    
    lua_getglobal(ls_, EXT_NAME);
    lua_pushstring(ls_, fieldname);
    lua_pushvalue(ls_, ADDR_ARG_POS(3));
    lua_settable(ls_, -3);
    lua_pop(ls_, 1);
    
    struct lambda_cb: public FFSlot::FFCallBack
    {
        lambda_cb(lua_State* lsaarg_, long idxarg):ls_(lsaarg_), idx(idxarg){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(HttpMgr::http_result_t))
            {
                return;
            }
            HttpMgr::http_result_t* data = (HttpMgr::http_result_t*)args_;

            Singleton<FFWorkerLua>::instance().getRpc().get_tq().produce(TaskBinder::gen(&lambda_cb::call_lua, ls_, idx, data->ret));
        }
        static void call_lua(lua_State* ls_, long idx, string retdata)
        {
            if (Singleton<FFWorkerLua>::instance().m_enable_call == false)
            {
                return;
            }
            char fieldname[256] = {0};
            snprintf(fieldname, sizeof(fieldname), "http#%ld", idx);
            
            lua_getglobal(ls_, EXT_NAME);
            lua_pushstring(ls_, fieldname);
            lua_gettable (ls_, -2);
            luacpp_op_t<string>::cpp2luastack(ls_, retdata);
            
            try
            {
                if (::lua_pcall(ls_, 1, 0, 0) != 0)
                {
                    string err = lua_err_handler_t::luatraceback(ls_, "lua_pcall faled func_name<%s>", fieldname);
                    ::lua_pop(ls_, 1);
                    throw lua_err_t(err);
                }
            }
            catch(exception& e_)
            {
                LOGERROR((FFWORKER_LUA, "workerobj_lua_t::gen_queryDB_callback exception<%s>", e_.what()));
            }
            
            lua_pushstring(ls_, fieldname);
            lua_pushnil(ls_);
            lua_settable(ls_, -3);
            lua_pop(ls_, 1);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(ls_, idx); }
        lua_State* ls_;
        long idx;
    };
    
    Singleton<FFWorkerLua>::instance().asyncHttp(url, timeoutsec,  new lambda_cb(ls_, idx));
    return 0;
}
static string lua_syncHttp(const string& url, int timeoutsec)
{
    return Singleton<FFWorkerLua>::instance().syncHttp(url, timeoutsec);
}
static void onSyncSharedData(int32_t cmd, const string& data){
    try{
        lua_args_t luaarg;
        luaarg.add((int64_t)cmd);
        luaarg.add(data);
        Singleton<FFWorkerLua>::instance().getFFlua().call<void>("onSyncSharedData", luaarg);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_LUA, "FFWorkerLua::onSyncSharedData exception=%s", e_.what()));
    }
}
static ScriptArgObjPtr toScriptArg(lua_State* ls_, int pos_){
    ScriptArgObjPtr ret = new ScriptArgObj();
    if (lua_isnumber(ls_, pos_)){
        double d  = lua_tonumber(ls_, pos_);
        int64_t n = (int64_t)d;
        if (::abs(d - n) > 0){
            ret->toFloat(d);
        }
        else{
            ret->toInt((int64_t)d);
        }
    }
    else if (lua_isnil(ls_, pos_)){
        return ret;
    }
    else if (lua_isboolean(ls_, pos_)){
        bool param_ = (bool)lua_toboolean(ls_, pos_);
        int n = (param_? 1: 0);
        ret->toInt(n);
    }
    else if (lua_isstring(ls_, pos_)){
        lua_pushvalue(ls_, pos_);
        size_t len  = 0;
        const char* src = lua_tolstring(ls_, -1, &len);
        string sval(src, len);
        lua_pop(ls_, 1);
        ret->toString(sval);
    }
    else if (lua_istable(ls_, pos_)){
        ret->toDict();
        
        lua_pushnil(ls_);
    	int real_pos = pos_;
    	if (pos_ < 0) real_pos = real_pos - 1;
        
        bool isList = true;
        int nIndex  = 1;
        while (lua_next(ls_, real_pos) != 0)
		{
            if (!lua_isnumber(ls_, -2)){
                isList = false;
                lua_pop(ls_, 1);
                continue;
            }
            if (nIndex != (int)lua_tonumber(ls_, -2)){
                isList = false;
                lua_pop(ls_, 1);
                continue;
            }
            ++nIndex;
			lua_pop(ls_, 1);
		}
        if (isList){
            ret->toList();
        }
        lua_pushnil(ls_);
		while (lua_next(ls_, real_pos) != 0)
		{
            ScriptArgObjPtr pval = toScriptArg(ls_, -1);
            if (isList){
                ret->listVal.push_back(pval);
            }
            else{
                string strKey;
                lua_pushvalue(ls_, -2);
                size_t len  = 0;
                const char* src = lua_tolstring(ls_, -1, &len);
                strKey.assign(src, len);
                lua_pop(ls_, 1);
                ret->dictVal[strKey] = pval;
            }
			
			lua_pop(ls_, 1);
		}
    }
    return ret;
}

static void fromScriptArgToLua(lua_State* ls_, ScriptArgObjPtr pvalue){
    if (!pvalue){
        lua_pushnil(ls_);
    }
    if (pvalue->isNull()){
        lua_pushnil(ls_);
    }
    else if (pvalue->isInt()){
        lua_pushnumber(ls_, (lua_Number)pvalue->getInt());
    }
    else if (pvalue->isFloat()){
        lua_pushnumber(ls_, (lua_Number)pvalue->getFloat());
    }
    else if (pvalue->isString()){
        lua_pushlstring(ls_, pvalue->getString().c_str(), pvalue->getString().size());
    }
    else if (pvalue->isList()){
        lua_newtable(ls_);
        
        for (size_t i = 0; i < pvalue->listVal.size(); ++i){
            lua_pushnumber(ls_, (lua_Number)(i+1));
            fromScriptArgToLua(ls_, pvalue->listVal[i]);
            lua_settable(ls_, -3);
        }
    }
    else if (pvalue->isDict()){
        lua_newtable(ls_);
        map<string, SharedPtr<ScriptArgObj> >::iterator it = pvalue->dictVal.begin();
        for (; it != pvalue->dictVal.end(); ++it)
        {
            lua_pushlstring(ls_, it->first.c_str(), it->first.size());
            fromScriptArgToLua(ls_, it->second);
            lua_settable(ls_, -3);
        }
    }else{
        lua_pushnil(ls_);
    }
    return;
}
static int lua_callFunc(lua_State* ls_){
    string funcName;
    luacpp_op_t<string>::lua2cpp(ls_, ADDR_ARG_POS(1), funcName);
    
    ScriptArgs scriptArgs;
    //!最多9个额外参数
    for (int i = 2; i <= 10; ++i){
        ScriptArgObjPtr elem = toScriptArg(ls_, ADDR_ARG_POS(i));
        if (elem->isNull()){
            break;
        }
        scriptArgs.args.push_back(elem);
    }
    LOGTRACE((FFWORKER_LUA, "lua_callFunc begin %s argsize=%d", funcName, scriptArgs.args.size()));
    if (false == SCRIPT_UTIL.callFunc(funcName, scriptArgs)){
        LOGERROR((FFWORKER_LUA, "lua_callFunc no funcname:%s", funcName));
        return 0;
    }
    fromScriptArgToLua(ls_, scriptArgs.getReturnValue());
    return 1;
}
static void pushLuaArgs(lua_State* ls_, void* pdata){
    ScriptArgs& varScript = *((ScriptArgs*)pdata);
    for (size_t i = 0; i < varScript.args.size(); ++i){
        fromScriptArgToLua(ls_, varScript.at(i));
    }
}
namespace ff{
template<> struct luacpp_op_t<ScriptArgObjPtr>
{
	static int luareturn2cpp(lua_State* ls_, int pos_, ScriptArgObjPtr& param_)
	{
        param_ = toScriptArg(ls_, pos_);
        //printf("param_:%d\n", param_->getInt());
		return 0;
	}
};
}
static bool callScriptImpl(const std::string& funcName, ScriptArgs& varScript){
    if (!Singleton<FFWorkerLua>::instance().m_enable_call)
    {
        return false;
    }
    
    lua_args_t luaarg;
    luaarg.arg_num = varScript.args.size();
    luaarg.pdata = &varScript;
    luaarg.func = pushLuaArgs;
    luaops_t& fflua = Singleton<FFWorkerLua>::instance().getFFlua();
    
    std::string exceptInfo;
    try{
        varScript.ret = fflua.call<ScriptArgObjPtr>(funcName, luaarg);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_LUA, "FFWorkerLua::callScriptImpl exception=%s", e_.what()));
        exceptInfo = e_.what();
    }
    if (exceptInfo.empty() == false && SCRIPT_UTIL.isExceptEnable()){ 
        throw std::runtime_error(exceptInfo);
    }
    return true;
}

struct ffext_t{};

static bool  lua_reg(lua_State* ls)
{                 ;             
    lua_reg_func_t(ls, EXT_NAME)
                 .def(&FFDb::escape, "escape")
                 
                 .def(&lua_send_msg_session, "sessionSendMsg")
                 .def(&lua_broadcast_msg_gate, "gateBroadcastMsg")
                 .def(&lua_multicast_msg_session, "sessionMulticastMsg")
                 .def(&lua_closeSession, "sessionClose")
                 .def(&lua_change_session_worker, "sessionChangeWorker")
                 .def(&lua_getSessionGate, "getSessionGate")
                 .def(&lua_getSessionIp, "getSessionIp")
                 .def(&lua_isExist, "isExist")
                 .def(&lua_reload, "reload")
                 .def(&lua_log, "log")
                 .def(&lua_regTimer, "regTimer")
                 .def(&lua_connectDB, "connectDB")
                 .def(&lua_asyncQuery, "asyncQuery")
                 .def(&lua_query, "query")
                 .def(&lua_asyncQueryByName, "asyncQueryByName")
                 .def(&lua_queryByName, "queryByName")
                 .def(&lua_workerRPC, "workerRPC")
                 .def(&lua_syncSharedData, "syncSharedData")
                 .def(&lua_asyncHttp, "asyncHttp")
                 .def(&lua_syncHttp, "syncHttp")
                 .def(&lua_writeLockGuard, "writeLockGuard")
                 .def(&lua_callFunc, "callFunc")
                 ;
    return true;
}

int FFWorkerLua::scriptInit(const string& lua_root)
{
    string path;
    std::size_t pos = lua_root.find_last_of("/");
    if (pos != string::npos)
    {
        path = lua_root.substr(0, pos+1);
        m_ext_name = lua_root.substr(pos+1, lua_root.size() - pos - 1);
        getFFlua().add_package_path(path);
    }
    else{
        m_ext_name = lua_root;
        getFFlua().add_package_path("/");
    }
    //pos = m_ext_name.find(".lua");
    //m_ext_name = m_ext_name.substr(0, pos);
    
    LOGTRACE((FFWORKER_LUA, "FFWorkerLua::scriptInit begin path:%s, m_ext_name:%s", path, m_ext_name));

    getSharedMem().setNotifyFunc(onSyncSharedData);
    (*m_fflua).reg(lua_reg);


    DB_MGR.start();
    ArgHelper& arg_helper = Singleton<ArgHelper>::instance();
    if (arg_helper.isEnableOption("-db")){
        int nDbNum = DB_THREAD_NUM;
        if (arg_helper.getOptionValue("-db").find("sqlite://") != std::string::npos){
            nDbNum = 1;
        }
        for (int i = 0; i < nDbNum; ++i){
            if (0 == DB_MGR.connectDB(arg_helper.getOptionValue("-db"), DB_DEFAULT_NAME)){
                LOGERROR((FFWORKER_LUA, "db connect failed"));
                return -1;
                break;
            }
        }
    }
    
    int ret = -2;
    
    try{
        
        Mutex                    mutex;
        ConditionVar            cond(mutex);
        
        getRpc().get_tq().produce(TaskBinder::gen(&FFWorkerLua::processInit, this, &mutex, &cond, &ret));
        LockGuard lock(mutex);
        if (ret == -2){
            cond.wait();
        }
        if (ret < 0)
        {
            this->close();
            return -1;
        }
    }
    catch(exception& e_)
    {
        return -1;
    }
    m_started = true;
    LOGTRACE((FFWORKER_LUA, "FFWorkerLua::scriptInit end ok"));
    return ret;
}
//!!处理初始化逻辑
int FFWorkerLua::processInit(Mutex* mutex, ConditionVar* var, int* ret)
{
    try{
        (*m_fflua).do_file(m_ext_name);
        SCRIPT_UTIL.setCallScriptFunc(callScriptImpl);
        if (this->initModule()){
            *ret = 0;

            lua_args_t luaarg;
            (*m_fflua).call<void>("init", luaarg);
        }
        else
            *ret = -1;
    }
    catch(exception& e_)
    {
        *ret = -1;
        LOGERROR((FFWORKER_LUA, "FFWorkerLua::open failed er=<%s>", e_.what()));
    }
    LockGuard lock(*mutex);
    var->signal();
    return 0;
}
void FFWorkerLua::scriptCleanup()
{
    try
    {
        lua_args_t luaarg;
        (*m_fflua).call<void>("cleanup", luaarg);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_LUA, "scriptCleanup failed er=<%s>", e_.what()));
    }
    this->cleanupModule();
    m_enable_call = false;
    DB_MGR.stop();
}
int FFWorkerLua::close()
{
    getRpc().get_tq().produce(TaskBinder::gen(&FFWorkerLua::scriptCleanup, this));
    FFWorker::close();
    if (false == m_started)
        return 0;
    m_started = false;

    return 0;
}

string FFWorkerLua::reload(const string& name_)
{
    AUTO_PERF();
    LOGTRACE((FFWORKER_LUA, "FFWorkerLua::reload begin name_[%s]", name_));
    return "not supported";
}
void FFWorkerLua::pylog(int level_, const string& mod_, const string& content_)
{
    switch (level_)
    {
        case 1:
        {
            LOGFATAL((mod_.c_str(), "%s", content_));
        }
        break;
        case 2:
        {
            LOGERROR((mod_.c_str(), "%s", content_));
        }
        break;
        case 3:
        {
            LOGWARN((mod_.c_str(), "%s", content_));
        }
        break;
        case 4:
        {
            LOGINFO((mod_.c_str(), "%s", content_));
        }
        break;
        case 5:
        {
            LOGDEBUG((mod_.c_str(), "%s", content_));
        }
        break;
        case 6:
        {
            LOGTRACE((mod_.c_str(), "%s", content_));
        }
        break;
        default:
        {
            LOGTRACE((mod_.c_str(), "%s", content_));
        }
        break;
    }
}

int FFWorkerLua::onSessionReq(userid_t session_id, uint16_t cmd, const std::string& data)
{
    try
    {
        lua_args_t luaarg;
        luaarg.add(session_id);
        luaarg.add((int64_t)cmd);
        luaarg.add(data);
        (*m_fflua).call<void>("onSessionReq", luaarg);//
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_LUA, "onSessionReq failed er=<%s>", e_.what()));
    }
    return 0;
}

int FFWorkerLua::onSessionOffline(userid_t session_id)
{
    try
    {
        lua_args_t luaarg;
        luaarg.add(session_id);
        (*m_fflua).call<void>("onSessionOffline", luaarg);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_LUA, "onSessionOffline failed er=<%s>", e_.what()));
    }
    return 0;
}
int FFWorkerLua::FFWorkerLua::onSessionEnter(userid_t session_id, const std::string& extra_data)
{
    try
    {
        lua_args_t luaarg;
        luaarg.add(session_id);
        luaarg.add(extra_data);
        (*m_fflua).call<void>("onSessionEnter", luaarg);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_LUA, "onSessionEnter failed er=<%s>", e_.what()));
    }
    return 0;
}

string FFWorkerLua::onWorkerCall(uint16_t cmd, const std::string& body)
{
    string ret;
    try
    {
        lua_args_t luaarg;
        luaarg.add((int64_t)cmd);
        luaarg.add(body);
        ret =(*m_fflua).call<string>("onWorkerCall", luaarg);
    }
    catch(exception& e_)
    {
        ret = "!";
        ret += e_.what();
        LOGERROR((FFWORKER_LUA, "onWorkerCall failed er=<%s>", ret));
    }
    return ret;
}

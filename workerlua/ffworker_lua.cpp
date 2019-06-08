
//脚本

#include "./ffworker_lua.h"
#include "base/perf_monitor.h"
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
static int64_t gScriptLambdaIdx = 0;
static void lambdaCallBack(lua_State* ls_, long idx, ScriptArgObjPtr funcArg)
{
    char fieldname[256] = {0};
    snprintf(fieldname, sizeof(fieldname), "h2cb#%ld", idx);

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
        LOGERROR((FFWORKER_LUA, "FFWorkerLua::lambdaCallBack exception<%s>", e_.what()));
    }

    lua_pushstring(ls_, fieldname);
    lua_pushnil(ls_);
    lua_settable(ls_, -3);
    lua_pop(ls_, 1);
}

static ScriptArgObjPtr toScriptArg(lua_State* ls_, int pos_){
    ScriptArgObjPtr ret = new ScriptArgObj();
    if (lua_isnumber(ls_, pos_)){
        double d  = lua_tonumber(ls_, pos_);
        int64_t n = (int64_t)d;
        if (abs((int64_t)d - n) > 0){
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
    else if (lua_isfunction(ls_, pos_)){
        char fieldname[256] = {0};
        ++gScriptLambdaIdx;
        long idx = long(gScriptLambdaIdx);
        snprintf(fieldname, sizeof(fieldname), "h2cb#%ld", idx);

        lua_getglobal(ls_, EXT_NAME);
        lua_pushstring(ls_, fieldname);
        lua_pushvalue(ls_, pos_);
        lua_settable(ls_, -3);
        lua_pop(ls_, 1);
        ret->toFunc(funcbind(&lambdaCallBack, ls_, idx));
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

static bool  lua_reg(lua_State* ls)
{
    lua_reg_func_t(ls, EXT_NAME).def(&lua_callFunc, "call");
    return true;
}

int FFWorkerLua::scriptInit(const string& lua_root)
{
    string path;
    std::size_t pos = lua_root.find_last_of("/");
    if (pos != string::npos)
    {
        path = lua_root.substr(0, pos+1);
        m_ext_name = lua_root;//lua_root.substr(pos+1, lua_root.size() - pos - 1);
        getFFlua().add_package_path(path);
    }
    else{
        m_ext_name = lua_root;
        getFFlua().add_package_path("/");
    }
    //pos = m_ext_name.find(".lua");
    //m_ext_name = m_ext_name.substr(0, pos);

    LOGINFO((FFWORKER_LUA, "FFWorkerLua::scriptInit begin path:%s, m_ext_name:%s", path, m_ext_name));

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

        getRpc().getTaskQueue().post(funcbind(&FFWorkerLua::processInit, this, &mutex, &cond, &ret));
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
    logtrace("FFWorkerLua::processInit begin!!!");
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
    LOGTRACE((FFWORKER_LUA, "scriptCleanup begin"));
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
    LOGTRACE((FFWORKER_LUA, "scriptCleanup trace 1"));
    m_enable_call = false;
    DB_MGR.stop();
    LOGTRACE((FFWORKER_LUA, "scriptCleanup end ok"));
}
int FFWorkerLua::close()
{
    LOGTRACE((FFWORKER_LUA, "close begin"));
    getRpc().getTaskQueue().post(funcbind(&FFWorkerLua::scriptCleanup, this));
    LOGTRACE((FFWORKER_LUA, "close trace 1"));
    FFWorker::close();
    LOGTRACE((FFWORKER_LUA, "close trace 2"));
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

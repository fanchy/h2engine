
//脚本

#include "ffworker_js.h"
#include "base/performance_daemon.h"
#include "server/http_mgr.h"
#include "base/os_tool.h"
 
using namespace ff;
using namespace std;

 using namespace v8;
 

FFWorkerJs::FFWorkerJs():
    m_enable_call(true), m_started(false),m_isolate(NULL)
{
    
}
FFWorkerJs::~FFWorkerJs()
{
}

#define CHECK_ARG_NUM(args, N) if (args.Length() < N) {BIND_FUNC_RET_UNDEFINED;} HANDLE_SCOPE_DEF_VAR;
static const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

static string repor_exception(v8::TryCatch& try_catch) {
  v8::HandleScope handle_scope();
  v8::String::Utf8Value exception(try_catch.Exception());
  
  const char* exception_string = ToCString(exception);
  v8::Local<v8::Message> message = try_catch.Message();
  string retlog;
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    retlog += exception_string;
  } else {
    #if V8_MAJOR_VERSION <= 3
        v8::Local<v8::Value> stack_trace_string = GetTryCatchStackTrace(try_catch);
    #elif V8_MAJOR_VERSION <= 4
        v8::Local<v8::Value> stack_trace_string = GetTryCatchStackTrace(try_catch);
        v8::Isolate* isolate = Singleton<FFWorkerJs>::instance().m_isolate;
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
    #else
        v8::Isolate* isolate = Singleton<FFWorkerJs>::instance().m_isolate;
        v8::Local<v8::Value> stack_trace_string;
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
        try_catch.StackTrace(context).ToLocal(&stack_trace_string);
    #endif
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(GetMessageResourceNam(message));
    
    string filename_string = ToCString(filename);
    if (filename_string == "undefined"){
        filename_string.clear();
    }
    int linenum = GetMessageLineNum(message);
    char msg[512] = {0};
    snprintf(msg, sizeof(msg), "%s:%i: %s\n", filename_string.c_str(), linenum, exception_string);
    retlog += msg;
    // Print line of source code.
    v8::String::Utf8Value sourceline(
        GetMessageSourceLine(message));
    const char* sourceline_string = ToCString(sourceline);
    snprintf(msg, sizeof(msg), "%s\n", sourceline_string);
    retlog += msg;
    // Print wavy underline (GetUnderline is deprecated).
    int start = GetMessageStartColumn(message);
    for (int i = 0; i < start; i++) {
      snprintf(msg, sizeof(msg), " ");
      retlog += msg;
    }
    int end = GetMessageEndColumn(message);
    for (int i = start; i < end; i++) {
      snprintf(msg, sizeof(msg), "^");
      retlog += msg;
    }
    snprintf(msg, sizeof(msg), "\n");
    retlog += msg;
    
    if (stack_trace_string.IsEmpty()==false &&
        stack_trace_string->IsString() &&
        v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
      v8::String::Utf8Value stack_trace(stack_trace_string);
      const char* stack_trace_string = ToCString(stack_trace);
      snprintf(msg, sizeof(msg), "%s\n", stack_trace_string);
      retlog += msg;
    }
  }
  return retlog;
}

static string js2string(const Local<Value>& v)
{
    if (false == v->IsString())
        return ""; 
    Local<String> ls = v->ToString();
    String::Utf8Value s(ls);
    string  data((const char*)*s, s.length());
    return data;
}
static int64_t js2num(const Local<Value>& v){
    if (v->IsNumber())
    {
        return v->IntegerValue();
    }
    else if (v->IsString()){
        Local<String> ls = v->ToString();
        String::Utf8Value s(ls);
        return ::atol(*s);
    }
    return 0;
}

static bool requirejs(const string& js_root, Handle<Value>* ret = NULL)
{    
    // 创建一个句柄作用域 ( 在栈上 ) 
    HANDLE_SCOPE_DEF_VAR;
    
    TryCatch try_catch;
    
    string filename = js_root;
    if (OSTool::isFile(filename) == false){
        filename = Singleton<FFWorkerJs>::instance().m_jspath + filename;
        if (OSTool::isFile(filename) == false){
            LOGERROR((FFWORKER_JS, "FFWorkerJs::requirejs exception=no file:%s", filename));
            return false;
        }
    }

    string filedata;
    if (false == OSTool::readFile(js_root, filedata)){
        LOGERROR((FFWORKER_JS, "FFWorkerJs::requirejs exception=no file:%s", filename));
        return false;
    }
    Handle<String> source = NewStrValue(filedata.c_str(), filedata.size());
    // 创建一个字符串对象，值为'Hello, Wrold!', 字符串对象被 JS 引擎
    // 求值后，结果为'Hello, World!'
    //Handle<String> source = NewStrValue("'Hello' + ', World!'"); 

    // 编译字符串对象为脚本对象
    Handle<String> filenameval = NewStrValue(filename.c_str(), filename.size());
    v8::ScriptOrigin origin(filenameval);
    
    Handle<Script> script = Script::Compile(source, &origin); 
    if (script.IsEmpty()){
        LOGERROR((FFWORKER_JS, "FFWorkerJs::requirejs exception=%s", repor_exception(try_catch)));
        return false;
    }

    // 执行脚本，获取结果
    Handle <Value> result = script->Run(); 
    if (result.IsEmpty()){
        LOGERROR((FFWORKER_JS, "FFWorkerJs::requirejs result exception=%s", repor_exception(try_catch)));
        return false;
    }
    if (ret){
        *ret = result;
    }
    LOGINFO((FFWORKER_JS, "FFWorkerJs::requirejs ok=%s", js_root));
    return true;
}
static BIND_FUNC_RET_TYPE js_require(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string  name        = js2string(args[0]);
    Handle<Value> ret;
    if (false == requirejs(name, &ret)){
        BIND_FUNC_RET_UNDEFINED;
    }
    BIND_FUNC_RET_VAL(ret);
}
static BIND_FUNC_RET_TYPE js_isFile(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string  name        = js2string(args[0]);

    if (OSTool::isFile(name)){
        BIND_FUNC_RET_TRUE;
    }
    BIND_FUNC_RET_FALSE;
}
static BIND_FUNC_RET_TYPE js_isDir(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string  name        = js2string(args[0]);

    if (OSTool::isDir(name)){
        BIND_FUNC_RET_TRUE;
    }
    BIND_FUNC_RET_FALSE;
}
static BIND_FUNC_RET_TYPE js_ls(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string  path        = js2string(args[0]);

    vector<string> files;
    if (OSTool::ls(path, files)){
        Local<Array> row_array = ARRAY_NEW(files.size());
        for (size_t j = 0; j < files.size(); ++j){
            Handle<String> v = NewStrValue(files[j].c_str(), files[j].size());
            row_array->Set(j, v);
        }
        BIND_FUNC_RET_VAL(row_array);
    }
    BIND_FUNC_RET_FALSE;
}
static BIND_FUNC_RET_TYPE js_readFile(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string  path        = js2string(args[0]);

    string data;
    if (OSTool::readFile(path, data)){
        Handle<String> v = NewStrValue(data.c_str(), data.size());
        BIND_FUNC_RET_VAL(v);
    }
    BIND_FUNC_RET_FALSE;
}
static BIND_FUNC_RET_TYPE js_escape(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string  ret        = FFDb::escape(js2string(args[0]));
    BIND_FUNC_RET_VAL(NewStrValue(ret.c_str(), ret.size()));
}

static BIND_FUNC_RET_TYPE js_print(const Arguments& args)
{
    bool first = true;
    for (int i = 0; i < args.Length(); i++)
    {
        HANDLE_SCOPE_DEF_VAR;
        if (first)
        {
            first = false;
        }
        else
        {
            printf(" ");
        }
        //convert the args[i] type to normal char* string

        v8::String::Utf8Value str(args[i]);
        printf("%s", *str);
    }
    printf("\n");
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_send_msg_session(const Arguments& args)
{
    CHECK_ARG_NUM(args, 3);
    
    userid_t session_id_ = (userid_t)js2num(args[0]);
    uint16_t cmd_        = (uint16_t)js2num(args[1]);
    string  data_        = js2string(args[2]);
    Singleton<FFWorkerJs>::instance().sessionSendMsg(session_id_, cmd_, data_);
    
    LOGTRACE((FFWORKER_JS, "js_send_msg_session %d %d %s", session_id_, cmd_, data_));
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_broadcast_msg_gate(const Arguments& args)
{
    CHECK_ARG_NUM(args, 2);
    uint16_t cmd_        = (uint16_t)js2num(args[0]);
    string  data_        = js2string(args[1]);
    
    Singleton<FFWorkerJs>::instance().gateBroadcastMsg(cmd_, data_);
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_sessionMulticastMsg(const Arguments& args)
{
    CHECK_ARG_NUM(args, 3);
    vector<userid_t> session_id_;
    if (args[0]->IsArray()){
        
        Array* pa = *args[0].As<Array>();
        uint32_t size = pa->Length();
        
        for (uint32_t i = 0; i < size; ++i){
            Local<Value> lo = pa->Get(i);
            userid_t sid = (userid_t)js2num(lo);
            sid = lo->IntegerValue();
            session_id_.push_back(sid);
        }
    }
    uint16_t cmd_        = (uint16_t)js2num(args[10]);
    string  data_        = js2string(args[2]);
    Singleton<FFWorkerJs>::instance().sessionMulticastMsg(session_id_, cmd_, data_);
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_sessionClose(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    userid_t session_id_ = (userid_t)js2num(args[0]);
    Singleton<FFWorkerJs>::instance().sessionClose(session_id_);
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_sessionChangeWorker(const Arguments& args)
{
    CHECK_ARG_NUM(args, 3);
    
    userid_t session_id_ = (userid_t)js2num(args[0]);
    int to_worker_index_ = (int)js2num(args[1]);
    string  extra_data   = js2string(args[2]);
    
    Singleton<FFWorkerJs>::instance().sessionChangeWorker(session_id_, to_worker_index_, extra_data);
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_getSessionGate(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    userid_t session_id_ = (userid_t)js2num(args[0]);
    string ret = Singleton<FFWorkerJs>::instance().getSessionGate(session_id_);
    BIND_FUNC_RET_VAL(NewStrValue(ret.c_str(), ret.size()));
}
static BIND_FUNC_RET_TYPE js_getSessionIp(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    userid_t session_id_ = (userid_t)js2num(args[0]);
    string ret = Singleton<FFWorkerJs>::instance().getSessionIp(session_id_);
    BIND_FUNC_RET_VAL(NewStrValue(ret.c_str(), ret.size()));
}
//! 判断某个service是否存在
static BIND_FUNC_RET_TYPE js_isExist(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string  service_name_   = js2string(args[0]);
    bool b = Singleton<FFWorkerJs>::instance().getRpc().isExist(service_name_);
    if (b){
        BIND_FUNC_RET_TRUE;
    }
    BIND_FUNC_RET_FALSE;
}
static BIND_FUNC_RET_TYPE js_reload(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string  name_   = js2string(args[0]);
    string ret = Singleton<FFWorkerJs>::instance().reload(name_);
    BIND_FUNC_RET_VAL(NewStrValue(ret.c_str(), ret.size()));
}
static BIND_FUNC_RET_TYPE js_log_impl(const Arguments& args)
{
    CHECK_ARG_NUM(args, 3);
    int level = (int)js2num(args[0]);
    string mod_ = js2string(args[1]);
    string content_= js2string(args[2]);
    Singleton<FFWorkerJs>::instance().js_log(level, mod_, content_);
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_writeLockGuard(const Arguments& args){
    CHECK_ARG_NUM(args, 0);
    Singleton<FFWorkerJs>::instance().getSharedMem().writeLockGuard();
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_regTimer(const Arguments& args)
{
    CHECK_ARG_NUM(args, 2);
    int mstimeout_ = (int)js2num(args[0]);
    persistent_lambda_ptr_t funcptr;

    if (args[1]->IsFunction()){
        Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(args[1]);
        funcptr = new persistent_lambda_t(func);
    }
    
    struct lambda_cb
    {
        static void callback(persistent_lambda_ptr_t funcptr)
        {
            try
            {
                HANDLE_SCOPE_DEF_VAR;
                if(funcptr){
                    Singleton<FFWorkerJs>::instance().call(funcptr);
                }
                else{
                    LOGERROR((FFWORKER_JS, "ffscene_js_t::js_onceTimer no callback"));
                }
            }
            catch(exception& e_)
            {
                LOGERROR((FFWORKER_JS, "ffscene_js_t::js_onceTimer exception<%s>", e_.what()));
            }
        }
    };
    
        
    Singleton<FFWorkerJs>::instance().regTimer(mstimeout_, 
                TaskBinder::gen(&lambda_cb::callback, funcptr));
    
    BIND_FUNC_RET_TRUE;
}

//!数据库相关操作
static BIND_FUNC_RET_TYPE js_connectDB(const Arguments& args)
{
    CHECK_ARG_NUM(args, 2);
    string host_ = js2string(args[0]);
    string group_ = js2string(args[1]);
    
    long ret = DB_MGR.connectDB(host_, group_);
    BIND_FUNC_RET_VAL(NewNumberValue(ret));
}
struct AsyncQueryCB
{
    AsyncQueryCB(persistent_lambda_ptr_t f):funcptr(f){}
    void operator()(DbMgr::queryDBResult_t& result)
    {
        LOGINFO((FFWORKER_JS, "FFWorkerJs::call_js **********"));
        DbMgr::queryDBResult_t* data = (DbMgr::queryDBResult_t*)(&result);
        call_js(funcptr, data->errinfo, data->dataResult, data->fieldNames, data->affectedRows);
    }
    void call_js(persistent_lambda_ptr_t funcptr, string errinfo, vector<vector<string> > ret_, 
                        vector<string> col_, int affectedRows)
    {
        
        HANDLE_SCOPE_DEF_VAR;
        if(funcptr){
            Local<Object> retDict = OBJECT_NEW();
            {
                string key = "datas";
                Handle<String> hk = NewStrValue(key.c_str(), key.size());
                Local<Array> datas_array = ARRAY_NEW(ret_.size());
                for (size_t i = 0; i < ret_.size(); ++i){
                    Local<Array> row_array = ARRAY_NEW(ret_[i].size());
                    for (size_t j = 0; j < ret_[i].size(); ++j){
                        Handle<String> v = NewStrValue(ret_[i][j].c_str(), ret_[i][j].size());
                        row_array->Set(j, v);
                    }
                    datas_array->Set(i, row_array);
                }
                retDict->Set(hk, datas_array);
            }
            {
                string key = "fields";
                Handle<String> hk = NewStrValue(key.c_str(), key.size());
                Local<Array> datas_array = ARRAY_NEW(col_.size());
                for (size_t i = 0; i < col_.size(); ++i){
                    Handle<String> v = NewStrValue(col_[i].c_str(), col_[i].size());
                    datas_array->Set(i, v);
                }
                retDict->Set(hk, datas_array);
            }
            {
                string key = "errinfo";
                Handle<String> hk = NewStrValue(key.c_str(), key.size());
                Handle<String> v = NewStrValue(errinfo.c_str(), errinfo.size());
                retDict->Set(hk, v);
            }
            {
                string key = "affectedRows";
                Handle<String> hk = NewStrValue(key.c_str(), key.size());
                Handle<Value> v = NewNumberValue(affectedRows);
                retDict->Set(hk, v);
            }
            Handle<Value> argv[1];
            argv[0] = retDict;
            Singleton<FFWorkerJs>::instance().call(funcptr, 1, argv);
        }
        else{
            LOGERROR((FFWORKER_JS, "ffscene_js_t::js_asyncQuery no callback"));
        }
    }
    
    persistent_lambda_ptr_t funcptr;
};
static BIND_FUNC_RET_TYPE js_asyncQuery(const Arguments& args)
{
    CHECK_ARG_NUM(args, 3);
    long db_id_ = (long)js2num(args[0]);
    string sql_ = js2string(args[1]);
    persistent_lambda_ptr_t funcptr;

    if (args[2]->IsFunction()){
        Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(args[2]);
        funcptr = new persistent_lambda_t(func);
    }
    
    AsyncQueryCB cb(funcptr);
    DB_MGR.asyncQueryModId(db_id_, sql_,  cb, &(Singleton<FFWorkerJs>::instance().getRpc().get_tq()));
    BIND_FUNC_RET_TRUE;
}
struct AsyncQueryNameCB
{
    AsyncQueryNameCB(persistent_lambda_ptr_t f):funcptr(f){}
    void operator()(DbMgr::queryDBResult_t& result)
    {
        DbMgr::queryDBResult_t* data = (DbMgr::queryDBResult_t*)(&result);

        call_js(funcptr, data->errinfo, data->dataResult, data->fieldNames, data->affectedRows);
    }
    void call_js(persistent_lambda_ptr_t funcptr, string errinfo, vector<vector<string> > ret_, 
                        vector<string> col_, int affectedRows)
    {
        HANDLE_SCOPE_DEF_VAR;
        if(funcptr){
            Local<Object> retDict = OBJECT_NEW();
            {
                string key = "datas";
                Handle<String> hk = NewStrValue(key.c_str(), key.size());
                Local<Array> datas_array = ARRAY_NEW(ret_.size());
                for (size_t i = 0; i < ret_.size(); ++i){
                    Local<Array> row_array = ARRAY_NEW(ret_[i].size());
                    for (size_t j = 0; j < ret_[i].size(); ++j){
                        Handle<String> v = NewStrValue(ret_[i][j].c_str(), ret_[i][j].size());
                        row_array->Set(j, v);
                    }
                    datas_array->Set(i, row_array);
                }
                retDict->Set(hk, datas_array);
            }
            {
                string key = "fields";
                Handle<String> hk = NewStrValue(key.c_str(), key.size());
                Local<Array> datas_array = ARRAY_NEW(col_.size());
                for (size_t i = 0; i < col_.size(); ++i){
                    Handle<String> v = NewStrValue(col_[i].c_str(), col_[i].size());
                    datas_array->Set(i, v);
                }
                retDict->Set(hk, datas_array);
            }
            {
                string key = "errinfo";
                Handle<String> hk = NewStrValue(key.c_str(), key.size());
                Handle<String> v = NewStrValue(errinfo.c_str(), errinfo.size());
                retDict->Set(hk, v);
            }
            {
                string key = "affectedRows";
                Handle<String> hk = NewStrValue(key.c_str(), key.size());
                Handle<Value> v = NewNumberValue(affectedRows);
                retDict->Set(hk, v);
            }
            Handle<Value> argv[1];
            argv[0] = retDict;
            Singleton<FFWorkerJs>::instance().call(funcptr, 1, argv);
        }
        else{
            LOGERROR((FFWORKER_JS, "ffscene_js_t::js_asyncQuery no callback"));
        }
    }
    persistent_lambda_ptr_t funcptr;
};
static BIND_FUNC_RET_TYPE js_asyncQueryByName(const Arguments& args)
{
    CHECK_ARG_NUM(args, 3);
    string group_ = js2string(args[0]);
    string sql_ = js2string(args[1]);
    persistent_lambda_ptr_t funcptr;

    if (args[2]->IsFunction()){
        Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(args[2]);
        funcptr = new persistent_lambda_t(func);
    }
    
    
    AsyncQueryNameCB cb(funcptr);
    DB_MGR.asyncQueryByName(group_, sql_,  cb, &(Singleton<FFWorkerJs>::instance().getRpc().get_tq()));
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_query(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    //long db_id_ = (long)js2num(args[0]);
    string sql_ = js2string(args[0]);
    
    string errinfo;
    vector<vector<string> > ret_;
    vector<string> col_;
    int affectedRows = 0;
    DB_MGR.query(sql_, &ret_, &errinfo, &affectedRows, &col_);
    

    Local<Object> retDict = OBJECT_NEW();
    {
        string key = "datas";
        Handle<String> hk = NewStrValue(key.c_str(), key.size());
        Local<Array> datas_array = ARRAY_NEW(ret_.size());
        for (size_t i = 0; i < ret_.size(); ++i){
            Local<Array> row_array = ARRAY_NEW(ret_[i].size());
            for (size_t j = 0; j < ret_[i].size(); ++j){
                Handle<String> v = NewStrValue(ret_[i][j].c_str(), ret_[i][j].size());
                row_array->Set(j, v);
            }
            datas_array->Set(i, row_array);
        }
        retDict->Set(hk, datas_array);
    }
    {
        string key = "fields";
        Handle<String> hk = NewStrValue(key.c_str(), key.size());
        Local<Array> datas_array = ARRAY_NEW(col_.size());
        for (size_t i = 0; i < col_.size(); ++i){
            Handle<String> v = NewStrValue(col_[i].c_str(), col_[i].size());
            datas_array->Set(i, v);
        }
        retDict->Set(hk, datas_array);
    }
    {
        string key = "errinfo";
        Handle<String> hk = NewStrValue(key.c_str(), key.size());
        Handle<String> v = NewStrValue(errinfo.c_str(), errinfo.size());
        retDict->Set(hk, v);
    }
    {
        string key = "affectedRows";
        Handle<String> hk = NewStrValue(key.c_str(), key.size());
        Handle<Value> v = NewNumberValue(affectedRows);
        retDict->Set(hk, v);
    }
    BIND_FUNC_RET_VAL(retDict);
}

static BIND_FUNC_RET_TYPE js_QueryByName(const Arguments& args)
{
    CHECK_ARG_NUM(args, 2);
    string group_ = js2string(args[0]);
    string sql_ = js2string(args[1]);
    
    string errinfo;
    vector<vector<string> > ret_;
    vector<string> col_;
    int affectedRows = 0;
    DB_MGR.queryByName(group_, sql_, &ret_, &errinfo, &affectedRows, &col_);
    

    Local<Object> retDict = OBJECT_NEW();
    {
        string key = "datas";
        Handle<String> hk = NewStrValue(key.c_str(), key.size());
        Local<Array> datas_array = ARRAY_NEW(ret_.size());
        for (size_t i = 0; i < ret_.size(); ++i){
            Local<Array> row_array = ARRAY_NEW(ret_[i].size());
            for (size_t j = 0; j < ret_[i].size(); ++j){
                Handle<String> v = NewStrValue(ret_[i][j].c_str(), ret_[i][j].size());
                row_array->Set(j, v);
            }
            datas_array->Set(i, row_array);
        }
        retDict->Set(hk, datas_array);
    }
    {
        string key = "fields";
        Handle<String> hk = NewStrValue(key.c_str(), key.size());
        Local<Array> datas_array = ARRAY_NEW(col_.size());
        for (size_t i = 0; i < col_.size(); ++i){
            Handle<String> v = NewStrValue(col_[i].c_str(), col_[i].size());
            datas_array->Set(i, v);
        }
        retDict->Set(hk, datas_array);
    }
    {
        string key = "errinfo";
        Handle<String> hk = NewStrValue(key.c_str(), key.size());
        Handle<String> v = NewStrValue(errinfo.c_str(), errinfo.size());
        retDict->Set(hk, v);
    }
    {
        string key = "affectedRows";
        Handle<String> hk = NewStrValue(key.c_str(), key.size());
        Handle<Value> v = NewNumberValue(affectedRows);
        retDict->Set(hk, v);
    }
    BIND_FUNC_RET_VAL(retDict);
}

//!调用其他worker的接口 
static BIND_FUNC_RET_TYPE js_workerRPC(const Arguments& args){   
    CHECK_ARG_NUM(args, 4);
    int workerindex = (int)js2num(args[0]);
    uint16_t cmd = (uint16_t)js2num(args[1]);
    string argdata = js2string(args[2]);
    
    persistent_lambda_ptr_t funcptr;
    if (args[3]->IsFunction()){
        Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(args[3]);
        funcptr = new persistent_lambda_t(func);
    }
    
    struct lambda_cb: public FFSlot::FFCallBack
    {
        lambda_cb(persistent_lambda_ptr_t f):funcptr(f){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(ffslot_req_arg))
            {
                return;
            }
            ffslot_req_arg* msg_data = (ffslot_req_arg*)args_;
            try
            {
                WorkerCallMsgt::out_t retmsg;
                try{
                    FFThrift::DecodeFromString(retmsg, msg_data->body);
                }
                catch(exception& e_)
                {
                }
                HANDLE_SCOPE_DEF_VAR;
                Handle<Value> argv[1];
                argv[0] = NewStrValue(retmsg.body.c_str(), retmsg.body.size());
                if(funcptr){
                    Singleton<FFWorkerJs>::instance().call(funcptr, 1, argv);
                }
            }
            catch(exception& e_)
            {
                LOGERROR((FFWORKER_JS, "ffscene_js_t::call_service exception=%s", e_.what()));
            }
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(funcptr); }
        persistent_lambda_ptr_t funcptr;
    };
    Singleton<FFWorkerJs>::instance().workerRPC(workerindex, cmd, argdata, new lambda_cb(funcptr));
    BIND_FUNC_RET_TRUE;
}

static BIND_FUNC_RET_TYPE js_syncSharedData(const Arguments& args)
{
    CHECK_ARG_NUM(args, 2);
    int cmd = (int)js2num(args[0]);
    string data = js2string(args[1]);
    Singleton<FFWorkerJs>::instance().getSharedMem().syncSharedData(cmd, data);
    BIND_FUNC_RET_TRUE;
}

static BIND_FUNC_RET_TYPE js_asyncHttp(const Arguments& args)
{
    CHECK_ARG_NUM(args, 3);
    string url_ = js2string(args[0]);
    int timeoutsec = (int)js2num(args[1]);
    
    persistent_lambda_ptr_t funcptr;
    if (args[2]->IsFunction()){
        Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(args[2]);
        funcptr = new persistent_lambda_t(func);
    }
    
    struct lambda_cb: public FFSlot::FFCallBack
    {
        lambda_cb(persistent_lambda_ptr_t f):funcptr(f){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(HttpMgr::http_result_t))
            {
                return;
            }
            HttpMgr::http_result_t* data = (HttpMgr::http_result_t*)args_;

            Singleton<FFWorkerJs>::instance().getRpc().get_tq().produce(TaskBinder::gen(&lambda_cb::call_js, funcptr, data->ret));
        }
        static void call_js(persistent_lambda_ptr_t funcptr, string retdata)
        {
            try
            {
                HANDLE_SCOPE_DEF_VAR;
                Handle<Value> argv[1];
                argv[0] = NewStrValue(retdata.c_str(), retdata.size());
                if(funcptr){
                    Singleton<FFWorkerJs>::instance().call(funcptr, 1, argv);
                }
            }
            catch(exception& e_)
            {
                LOGERROR((FFWORKER_JS, "ffscene_js_t::js_asyncHttp exception<%s>", e_.what()));
            }
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(funcptr); }
        persistent_lambda_ptr_t funcptr;
    };
    
    
    Singleton<FFWorkerJs>::instance().asyncHttp(url_, timeoutsec, new lambda_cb(funcptr));
    
    BIND_FUNC_RET_TRUE;
}
static BIND_FUNC_RET_TYPE js_syncHttp(const Arguments& args)
{
    CHECK_ARG_NUM(args, 2);
    string url_ = js2string(args[0]);
    int timeoutsec = (int)js2num(args[1]);
    
    std::string ret = Singleton<FFWorkerJs>::instance().syncHttp(url_, timeoutsec);
    
    BIND_FUNC_RET_VAL(NewStrValue(ret.c_str(), ret.size()));
}

static void onSyncSharedData(int32_t cmd, const string& data){

    try{
        HANDLE_SCOPE_DEF_VAR;
        Handle<Value> argv[2];
        argv[0] = NewNumberValue(cmd);
        argv[1] = NewStrValue(data.c_str(), data.size());
        Singleton<FFWorkerJs>::instance().call(Singleton<FFWorkerJs>::instance().m_func_req, 2, argv);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_JS, "FFWorkerJs::onSyncSharedData exception=%s", e_.what()));
    }
    
}

static ScriptArgObjPtr toScriptArg(Local<Value>& v){
    ScriptArgObjPtr ret = new ScriptArgObj();
    if (v->IsNumber()){
        double d  = v->NumberValue();
        char buff[64] = {0};
        snprintf(buff, sizeof(buff), "%g", d);
        if (::strstr(buff, ".") == NULL){
            ret->toInt((int64_t)v->IntegerValue());
        }
        else{
            ret->toFloat(d);
        }
    }
    else if (v->IsBoolean()){
        bool b = v->BooleanValue();
        if (b)
            ret->toInt(1);
        else
            ret->toInt(0);
    }
    else if (v->IsString()){
        Local<String> ls = v->ToString();
        String::Utf8Value s(ls);
        string  sval((const char*)*s, s.length());
        ret->toString(sval);
    }
    else if (v->IsArray()){
        ret->toList();
        Array* pa = *v.As<Array>();
        uint32_t size = pa->Length();
        
        for (uint32_t i = 0; i < size; ++i){
            Local<Value> lo = pa->Get(i);
            ScriptArgObjPtr elem = toScriptArg(lo);
            ret->listVal.push_back(elem);
        }
    }
    else if (v->IsObject()){
        ret->toDict();
        //Local<Object> pa = v.As<Object>();
    }
    return ret;
}
static Local<Value> fromScriptArgToJs(ScriptArgObjPtr pvalue){
    if (!pvalue){
        return NewNumberValue(0);
    }
    if (pvalue->isNull()){
        return NewNumberValue(0);
    }
    else if (pvalue->isInt()){
        return NewNumberValue(pvalue->getInt());
    }
    else if (pvalue->isFloat()){
        return NewNumberValue(pvalue->getFloat());
    }
    else if (pvalue->isString()){
        return NewStrValue(pvalue->getString().c_str(), pvalue->getString().size());
    }
    else if (pvalue->isList()){
        Local<Array> row_array = ARRAY_NEW(pvalue->listVal.size());
        for (size_t j = 0; j < pvalue->listVal.size(); ++j){
            Handle<Value> v = fromScriptArgToJs(pvalue->listVal[j]);
            row_array->Set(j, v);
        }
        return row_array;
    }
    else if (pvalue->isDict()){
        Local<Object> retDict = OBJECT_NEW();
        map<string, SharedPtr<ScriptArgObj> >::iterator it = pvalue->dictVal.begin();
        for (; it != pvalue->dictVal.end(); ++it)
        {
            Handle<String> hk = NewStrValue(it->first.c_str(), it->first.size());
            Local<Value> dictVal = fromScriptArgToJs(it->second);
            retDict->Set(hk, dictVal);
        }
        return retDict;
    }
    return NewNumberValue(0);
}
static BIND_FUNC_RET_TYPE js_callFunc(const Arguments& args)
{
    CHECK_ARG_NUM(args, 1);
    string funcName = js2string(args[0]);
    
    ScriptArgs scriptArgs;
    for (int i = 1; i < args.Length(); ++i){
        Local<Value> v = args[i];
        ScriptArgObjPtr elem = toScriptArg(v);
        scriptArgs.args.push_back(elem);
    }
    LOGTRACE((FFWORKER_JS, "js_callFunc begin argsize=%d", scriptArgs.args.size()));
    if (false == SCRIPT_UTIL.callFunc(funcName, scriptArgs)){
        LOGERROR((FFWORKER_JS, "js_callFunc no funcname:%s", funcName));
        BIND_FUNC_RET_UNDEFINED;
    }
    Local<Value> ret = fromScriptArgToJs(scriptArgs.getReturnValue());
    BIND_FUNC_RET_VAL(ret);
}
static bool callScriptImpl(const std::string& funcNameArg, ScriptArgs& varScript){
    if (!Singleton<FFWorkerJs>::instance().m_enable_call)
    {
        return false;
    }
    std::string scriptName;
    std::string funcName;
    std::vector<std::string> vtFuncStr;
    StrTool::split(funcNameArg, vtFuncStr, ".");
    if (vtFuncStr.size() == 2){
        scriptName = vtFuncStr[0];
        funcName = vtFuncStr[1];
    }
    else{
        funcName = funcNameArg;
    }
    std::string exceptInfo;
    try{
        HANDLE_SCOPE_DEF_VAR;
        if (scriptName.empty()){
            Handle<v8::Value> func = PERSISTENT2LOCAL(Singleton<FFWorkerJs>::instance()._global_context)->Global()->Get(NewStrValue(funcName.c_str(), funcName.size()));
            if (!func->IsFunction()) {
                LOGERROR((FFWORKER_JS, "FFWorkerJs::callScriptImpl failed no func:%s", funcNameArg));
                exceptInfo = "callScriptImpl failed no func:";
                exceptInfo += funcNameArg;
            }
            else{
                Handle<Value> argv[9];
                for (size_t i = 0; i < 9 && i < varScript.args.size(); ++i)
                {
                    argv[i] = fromScriptArgToJs(varScript.args[i]);
                }
                persistent_lambda_ptr_t funcScript = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(func));
                Singleton<FFWorkerJs>::instance().call(funcScript, varScript.args.size(), argv, NULL, &(varScript.ret));
            }
        }
        else{
            Handle<v8::Value> objHandler = PERSISTENT2LOCAL(Singleton<FFWorkerJs>::instance()._global_context)->Global()->Get(NewStrValue(scriptName.c_str(), scriptName.size()));
            if (!objHandler->IsObject()) {
                LOGERROR((FFWORKER_JS, "FFWorkerJs::callScriptImpl failed no objHandler:%s", funcNameArg));
                exceptInfo = "callScriptImpl failed no obj:";
                exceptInfo += funcNameArg;
            }
            else{
                Handle<Object> pa = objHandler.As<Object>();
                Handle<v8::Value> func = PERSISTENT2LOCAL(pa->Get(NewStrValue(funcName.c_str(), funcName.size())));
                Handle<Value> argv[9];
                for (size_t i = 0; i < 9 && i < varScript.args.size(); ++i)
                {
                    argv[i] = fromScriptArgToJs(varScript.args[i]);
                }
                persistent_lambda_ptr_t funcScript = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(func));
                Singleton<FFWorkerJs>::instance().call(funcScript, varScript.args.size(), argv, NULL, &(varScript.ret));
            }
        }
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_JS, "FFWorkerJs::callScriptImpl exception=%s", e_.what()));
    }
    if (exceptInfo.empty() == false && SCRIPT_UTIL.isExceptEnable()){ 
        throw std::runtime_error(exceptInfo);
    }
    return true;
}

int FFWorkerJs::scriptInit(const string& js_root)
{
    string path;
    string ext_name;
    std::size_t pos = js_root.find_last_of("/");
    if (pos != string::npos)
    {
        path = js_root.substr(0, pos+1);
        ext_name = js_root.substr(pos+1, js_root.size() - pos - 1);
    }
    else{
        ext_name = js_root;
    }
    pos = ext_name.find(".js");
    ext_name = ext_name.substr(0, pos);
    m_jspath = path;
    LOGINFO((FFWORKER_JS, "FFWorkerJs::scriptInit begin path:%s, m_ext_name:%s", path, ext_name));
    
    getSharedMem().setNotifyFunc(onSyncSharedData);

    DB_MGR.start();
    ArgHelper& arg_helper = Singleton<ArgHelper>::instance();
    if (arg_helper.isEnableOption("-db")){
        int nDbNum = DB_THREAD_NUM;
        if (arg_helper.getOptionValue("-db").find("sqlite://") != std::string::npos){
            nDbNum = 1;
        }
        for (int i = 0; i < nDbNum; ++i){
            if (0 == DB_MGR.connectDB(arg_helper.getOptionValue("-db"), DB_DEFAULT_NAME)){
                LOGERROR((FFWORKER_JS, "db connect failed"));
                return -1;
                break;
            }
        }
    }
    
    int ret = -2;
    
    try{
        
        Mutex                    mutex;
        ConditionVar             cond(mutex);
        
        getRpc().get_tq().produce(TaskBinder::gen(&FFWorkerJs::processInit, this, &mutex, &cond, &ret, js_root));
        
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
    LOGINFO((FFWORKER_JS, "FFWorkerJs::scriptInit end ok"));
    return ret;
}

//!!处理初始化逻辑
int FFWorkerJs::processInit(Mutex* mutex, ConditionVar* var, int* ret, const string& js_root)
{
    try{
        m_v8init = new v8init_t();
        // 创建一个句柄作用域 ( 在栈上 ) 
        m_isolate = Isolate::New(Isolate_New_Param);
        m_isolate->Enter();
#if V8_MAJOR_VERSION <= 3
        _global_context = Context::New();
        PERSISTENT2LOCAL(_global_context)->Enter();//!Enter之后，就变成默认的Context
#endif
        HANDLE_SCOPE_DEF_VAR;
        {
            
            
            Handle<ObjectTemplate> global = ObjectTemplate::New(); 
            
            global->Set(NewStrValue2("require"),FUNCTIONTEMPLATE_NEW(js_require));
            
            global->Set(NewStrValue2("isFile"),FUNCTIONTEMPLATE_NEW(js_isFile));
            global->Set(NewStrValue2("isDir"),FUNCTIONTEMPLATE_NEW(js_isDir));
            global->Set(NewStrValue2("readFile"),FUNCTIONTEMPLATE_NEW(js_readFile));
            global->Set(NewStrValue2("ls"),FUNCTIONTEMPLATE_NEW(js_ls));
            global->Set(NewStrValue2("escape"),FUNCTIONTEMPLATE_NEW(js_escape));
            global->Set(NewStrValue2("print"),FUNCTIONTEMPLATE_NEW(js_print));
            global->Set(NewStrValue2("sessionSendMsg"),FUNCTIONTEMPLATE_NEW(js_send_msg_session));
            global->Set(NewStrValue2("gateBroadcastMsg"),FUNCTIONTEMPLATE_NEW(js_broadcast_msg_gate));
            global->Set(NewStrValue2("sessionMulticastMsg"),FUNCTIONTEMPLATE_NEW(js_sessionMulticastMsg));
            global->Set(NewStrValue2("sessionClose"),FUNCTIONTEMPLATE_NEW(js_sessionClose));
            global->Set(NewStrValue2("sessionChangeWorker"),FUNCTIONTEMPLATE_NEW(js_sessionChangeWorker));
            global->Set(NewStrValue2("getSessionGate"),FUNCTIONTEMPLATE_NEW(js_getSessionGate));
            global->Set(NewStrValue2("getSessionIp"),FUNCTIONTEMPLATE_NEW(js_getSessionIp));
            global->Set(NewStrValue2("isExist"),FUNCTIONTEMPLATE_NEW(js_isExist));
            global->Set(NewStrValue2("reload"),FUNCTIONTEMPLATE_NEW(js_reload));
            global->Set(NewStrValue2("log"),FUNCTIONTEMPLATE_NEW(js_log_impl));
            global->Set(NewStrValue2("regTimer"),FUNCTIONTEMPLATE_NEW(js_regTimer));

            global->Set(NewStrValue2("connectDB"),FUNCTIONTEMPLATE_NEW(js_connectDB));
            global->Set(NewStrValue2("asyncQuery"),FUNCTIONTEMPLATE_NEW(js_asyncQuery));
            global->Set(NewStrValue2("query"),FUNCTIONTEMPLATE_NEW(js_query));
            global->Set(NewStrValue2("asyncQueryByName"),FUNCTIONTEMPLATE_NEW(js_asyncQueryByName));
            global->Set(NewStrValue2("queryByName"),FUNCTIONTEMPLATE_NEW(js_QueryByName));
            global->Set(NewStrValue2("workerRPC"),FUNCTIONTEMPLATE_NEW(js_workerRPC));
            global->Set(NewStrValue2("syncSharedData"),FUNCTIONTEMPLATE_NEW(js_syncSharedData));
            global->Set(NewStrValue2("asyncHttp"),FUNCTIONTEMPLATE_NEW(js_asyncHttp));
            global->Set(NewStrValue2("syncHttp"),FUNCTIONTEMPLATE_NEW(js_syncHttp));
            global->Set(NewStrValue2("callFunc"),FUNCTIONTEMPLATE_NEW(js_callFunc));
            global->Set(NewStrValue2("writeLockGuard"),FUNCTIONTEMPLATE_NEW(js_writeLockGuard));
            
#if V8_MAJOR_VERSION <= 3
            PERSISTENT2LOCAL(_global_context)->Global()->Set(NewStrValue(EXT_NAME), global->NewInstance());
#else
            m_hcontext = v8::Context::New(m_isolate, NULL, global);
            _global_context.Reset(m_isolate, m_hcontext);
            PERSISTENT2LOCAL(_global_context)->Enter();//!Enter之后，就变成默认的Context
#endif
            
        }
        

        
        // 创建一个新的上下文对象
        
        // Enter the newly created execution environment. 

        if (false == requirejs(js_root)){
            *ret = -1;
        }
        else{
            
        
            {
                string funcname = "onSessionReq";
                Handle<v8::Value> value = PERSISTENT2LOCAL(_global_context)->Global()->Get(NewStrValue(funcname.c_str(), funcname.size()));
                if (value->IsFunction()) {
                    m_func_req = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(value));
                }
                else{
                    LOGERROR((FFWORKER_JS, "FFWorkerJs::open failed no onSessionReq"));
                }
            }
            {
                string funcname = "onSessionOffline";
                Handle<v8::Value> value = PERSISTENT2LOCAL(_global_context)->Global()->Get(NewStrValue(funcname.c_str(), funcname.size()));
                if (value->IsFunction()) {
                    m_func_offline = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(value));
                }
                else{
                    LOGERROR((FFWORKER_JS, "FFWorkerJs::open failed no onSessionOffline"));
                }
            }
            {
                string funcname = "onSessionEnter";
                Handle<v8::Value> value = PERSISTENT2LOCAL(_global_context)->Global()->Get(NewStrValue(funcname.c_str(), funcname.size()));
                if (value->IsFunction()) {
                    m_func_enter = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(value));
                }else{
                    LOGERROR((FFWORKER_JS, "FFWorkerJs:: no onSessionEnter"));
                }
            }
            {
                string funcname = "onWorkerCall";
                Handle<v8::Value> value = PERSISTENT2LOCAL(_global_context)->Global()->Get(NewStrValue(funcname.c_str(), funcname.size()));
                if (value->IsFunction()) {
                    m_func_rpc = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(value));
                }else{
                    LOGERROR((FFWORKER_JS, "FFWorkerJs::open failed no onWorkerCall"));
                }
            }
            {
                string funcname = "onSyncSharedData";
                Handle<v8::Value> value = PERSISTENT2LOCAL(_global_context)->Global()->Get(NewStrValue(funcname.c_str(), funcname.size()));
                if (value->IsFunction()) {
                    m_func_sync = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(value));
                }else{
                    LOGERROR((FFWORKER_JS, "FFWorkerJs:: no onSyncSharedData"));
                }
            }
            SCRIPT_UTIL.setCallScriptFunc(callScriptImpl);
            if (this->initModule()){
                *ret = 0;
                LOGINFO((FFWORKER_JS, "FFWorkerJs::processInit end ok0"));
                {
                    HANDLE_SCOPE_DEF_VAR;
                    string funcname = "init";
                    Handle<v8::Value> value = PERSISTENT2LOCAL(_global_context)->Global()->Get(NewStrValue(funcname.c_str(), funcname.size()));
                    if (value->IsFunction()) {
                        persistent_lambda_ptr_t pf = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(value));
                        call(pf);
                    }
                }
            }
            else
                *ret = -1;
            
        }
    }
    catch(exception& e_)
    {
        *ret = -1;
        LOGERROR((FFWORKER_JS, "FFWorkerJs::open failed er=<%s>", e_.what()));
    }
    LOGINFO((FFWORKER_JS, "FFWorkerJs::processInit end ok1"));
    LockGuard lock(*mutex);
    var->signal();

    LOGINFO((FFWORKER_JS, "FFWorkerJs::processInit end ok"));
    return 0;
}
void FFWorkerJs::scriptCleanup()
{
    {
        HANDLE_SCOPE_DEF_VAR;
        string funcname = "cleanup";
        Handle<v8::Value> value = PERSISTENT2LOCAL(_global_context)->Global()->Get(NewStrValue(funcname.c_str(), funcname.size()));
        if (value->IsFunction()) {
            persistent_lambda_ptr_t pf = new persistent_lambda_t(v8::Handle<v8::Function>::Cast(value));
            call(pf);
        }
    }
    this->cleanupModule();
    m_enable_call = false;
    DB_MGR.stop();
    getRpc().get_tq().produce(TaskBinder::gen(&FFWorkerJs::scriptRelease, this));
}

void FFWorkerJs::scriptRelease(){

    // 释放上下文资源
    m_func_req.reset();
    m_func_offline.reset();
    m_func_enter.reset();
    m_func_rpc.reset();
    m_func_sync.reset();
    PERSISTENT2LOCAL(_global_context)->Exit();
	ReaseV8Persistent(_global_context);
    
    if (m_isolate){
        m_isolate->Exit();
        m_isolate->Dispose();
        m_isolate = NULL;
    }
    m_v8init.reset();
}
int FFWorkerJs::close()
{
    LOGINFO((FFWORKER_JS, "FFWorkerJs::close begin"));
    getRpc().get_tq().produce(TaskBinder::gen(&FFWorkerJs::scriptCleanup, this));
    LOGINFO((FFWORKER_JS, "FFWorkerJs::close begin to close"));
    FFWorker::close();
    if (false == m_started)
        return 0;
    m_started = false;

    LOGINFO((FFWORKER_JS, "FFWorkerJs::close end"));
    return 0;
}

string FFWorkerJs::reload(const string& name_)
{
    /*
    AUTO_PERF();
    LOGTRACE((FFWORKER_JS, "FFWorkerJs::reload begin name_[%s]", name_));
    try
    {
        ffpython_t::reload(name_);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_JS, "FFWorkerJs::reload exeception=%s", e_.what()));
        return e_.what();
    }
    LOGTRACE((FFWORKER_JS, "FFWorkerJs::reload end ok name_[%s]", name_));
    */
    return "";
}
void FFWorkerJs::js_log(int level_, const string& mod_, const string& content_)
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

int FFWorkerJs::onSessionReq(userid_t session_id_, uint16_t cmd_, const std::string& data_)
{
    try
    {
        HANDLE_SCOPE_DEF_VAR;
        Handle<Value> argv[3];
        argv[0] = NewNumberValue(session_id_);
        argv[1] = NewNumberValue(cmd_);
        argv[2] = NewStrValue(data_.c_str(), data_.size());
        Singleton<FFWorkerJs>::instance().call(m_func_req, 3, argv);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_JS, "onSessionReq failed er=<%s>", e_.what()));
    }
    return 0;
}

int FFWorkerJs::onSessionOffline(userid_t session_id)
{
    try
    {
        HANDLE_SCOPE_DEF_VAR;
        Handle<Value> argv[1];
        argv[0] = NewNumberValue(session_id);
        Singleton<FFWorkerJs>::instance().call(m_func_offline, 1, argv);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_JS, "onSessionOffline failed er=<%s>", e_.what()));
    }
    return 0;
}
int FFWorkerJs::FFWorkerJs::onSessionEnter(userid_t session_id, const std::string& extra_data)
{
    try
    {
        HANDLE_SCOPE_DEF_VAR;
        Handle<Value> argv[2];
        argv[0] = NewNumberValue(session_id);
        argv[1] = NewStrValue(extra_data.c_str(), extra_data.size());
        Singleton<FFWorkerJs>::instance().call(m_func_enter, 2, argv);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_JS, "onSessionEnter failed er=<%s>", e_.what()));
    }
    return 0;
}

string FFWorkerJs::onWorkerCall(uint16_t cmd, const std::string& body)
{
    string ret;
    try
    {
        HANDLE_SCOPE_DEF_VAR;
        Handle<Value> argv[2];
        argv[0] = NewNumberValue(cmd);
        argv[1] = NewStrValue(body.c_str(), body.size());
        
        Singleton<FFWorkerJs>::instance().call(m_func_rpc, 2, argv, &ret);
    }
    catch(exception& e_)
    {
        LOGERROR((FFWORKER_JS, "onWorkerCall failed er=<%s>", e_.what()));
    }

    return ret;
}
bool FFWorkerJs::call(persistent_lambda_ptr_t& pf, int argc, Handle<Value>* argv, std::string* ret, ScriptArgObjPtr* pRet)
{
    if (m_enable_call == false){
        return false;
    }
    if (!pf){
        return false;
    }
    HANDLE_SCOPE_DEF_VAR;
    v8::Handle<v8::Function> jscb = pf->getfunc();
    if (jscb.IsEmpty() || false == jscb->IsCallable()){
        return false;
    }
    Handle<v8::Object> global = PERSISTENT2LOCAL(_global_context)->Global();
    TryCatch try_catch;

    Local <Value> result = jscb->Call(global, argc, argv);
    if (result.IsEmpty()){
        LOGERROR((FFWORKER_JS, "FFWorkerJs::call exception=%s", repor_exception(try_catch)));
        return false;
    }
    if (ret){
        *ret = js2string(result);
    }
    else if (pRet){
        *pRet = toScriptArg(result);
    }
    return true;
}


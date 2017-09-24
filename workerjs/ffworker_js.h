#ifndef _FF_FFWORKER_JS_H_
#define _FF_FFWORKER_JS_H_

//#include <assert.h>
//#include <string>


#include "base/log.h"
#include "server/db_mgr.h"
#include "server/fftask_processor.h"
#include "server/ffworker.h"
#include "server/script.h" 

#include <v8.h>
#ifndef V8_MAJOR_VERSION
    #define V8_MAJOR_VERSION 3 //!Centos 7 yum默认安装的是3.14.5，版本3是没有定义这个宏的，4开始后有
    
    
#else
    #include <libplatform/libplatform.h> //!4之后的版本需要platform作为参数
#endif

namespace ff
{
#define FFWORKER_JS "FFWORKER_JS"

#if V8_MAJOR_VERSION <= 3
    #define Isolate_New_Param
    struct v8init_t
    {
        
    };
    #define HANDLE_SCOPE_DEF_VAR HandleScope handle_scope
    #define NewNumberValue(v) Number::New(v)
    #define NewStrValue String::New
    #define NewStrValue2 String::New
    #define ReaseV8Persistent(x) (x).Dispose()
    #define GetMessageResourceNam(message) message->GetScriptResourceName()
    #define GetMessageLineNum(message) message->GetLineNumber()
    #define GetMessageSourceLine(message) message->GetSourceLine()
    #define GetMessageStartColumn(message) message->GetStartColumn()
    #define GetMessageEndColumn(message) message->GetEndColumn()
    #define GetTryCatchStackTrace(try_catch) try_catch.StackTrace()
    #define BIND_FUNC_RET_TYPE Handle<Value>
    #define BIND_FUNC_RET_UNDEFINED return Undefined()
    #define BIND_FUNC_RET_TRUE return True()
    #define BIND_FUNC_RET_FALSE return False()
    #define BIND_FUNC_RET_VAL(x) return x
    #define PERSISTENT2LOCAL(x) x
    #define ARRAY_NEW(x) Array::New(x)
    #define PERSISTENT_FUNCTION_RESET(x, y) x = v8::Persistent<v8::Function>::New(y)
    #define FUNCTIONTEMPLATE_NEW(x) FunctionTemplate::New(x)
    #define OBJECT_NEW() Object::New()
#else
    #define Isolate_New_Param m_v8init->create_params
    #define HANDLE_SCOPE_DEF_VAR HandleScope handle_scope(Singleton<FFWorkerJs>::instance().m_isolate)
    #define Arguments v8::FunctionCallbackInfo<v8::Value>
    #define NewNumberValue(v) Number::New(Singleton<FFWorkerJs>::instance().m_isolate, v)
    #define NewStrValue(v, n) String::NewFromUtf8(Singleton<FFWorkerJs>::instance().m_isolate, v, v8::String::kNormalString, n)
    #define NewStrValue2(v) String::NewFromUtf8(Singleton<FFWorkerJs>::instance().m_isolate, v)
    
    #define ReaseV8Persistent(x) (x).Reset()
    #define GetMessageResourceNam(message) message->GetScriptOrigin().ResourceName()
    #define GetMessageLineNum(message) message->GetLineNumber(context).FromJust()
    #define GetMessageSourceLine(message) message->GetSourceLine(context).ToLocalChecked()
    #define GetMessageStartColumn(message) message->GetStartColumn(context).FromJust()
    #define GetMessageEndColumn(message) message->GetEndColumn(context).FromJust()
    
    #define BIND_FUNC_RET_TYPE void
    #define BIND_FUNC_RET_UNDEFINED     args.GetReturnValue().Set(Undefined(Singleton<FFWorkerJs>::instance().m_isolate));return
    #define BIND_FUNC_RET_TRUE          args.GetReturnValue().Set(True(Singleton<FFWorkerJs>::instance().m_isolate));return
    #define BIND_FUNC_RET_FALSE         args.GetReturnValue().Set(False(Singleton<FFWorkerJs>::instance().m_isolate));return
    #define BIND_FUNC_RET_VAL(x)        args.GetReturnValue().Set(x);return
    
    #define PERSISTENT2LOCAL(x) m_hcontext
    
    #define ARRAY_NEW(x) Array::New(Singleton<FFWorkerJs>::instance().m_isolate, x)
    #define PERSISTENT_FUNCTION_RESET(x, y) x.Reset(Singleton<FFWorkerJs>::instance().m_isolate, y)
    #define FUNCTIONTEMPLATE_NEW(x) FunctionTemplate::New(Singleton<FFWorkerJs>::instance().m_isolate, x)
    #define OBJECT_NEW() Object::New(Singleton<FFWorkerJs>::instance().m_isolate)
    
    #if V8_MAJOR_VERSION <= 4
        #define GetTryCatchStackTrace(try_catch) try_catch.StackTrace()
    #endif
    
    #if V8_MAJOR_VERSION <= 5
        #define GetTryCatchStackTrace(try_catch) try_catch.StackTrace()
        class ShellArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
         public:
          virtual void* Allocate(size_t length) {
            void* data = AllocateUninitialized(length);
            return data == NULL ? data : memset(data, 0, length);
          }
          virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
          virtual void Free(void* data, size_t) { free(data); }
        };
        struct v8init_t
        {
            v8init_t(){
                int argc = 0;
                char* argv[1];
                argv[0] = (char*)"h2js";
                
                
                v8::V8::InitializeICU();
                #if V8_MAJOR_VERSION == 5
                    v8::V8::InitializeExternalStartupData(argv[0]);
                #endif
                platform = v8::platform::CreateDefaultPlatform();
                v8::V8::InitializePlatform(platform);
                v8::V8::Initialize();
                v8::V8::SetFlagsFromCommandLine(&argc, argv, true);

                create_params.array_buffer_allocator = &array_buffer_allocator;
            }
            ~v8init_t(){
                v8::V8::Dispose();
                v8::V8::ShutdownPlatform();
                delete platform;
                platform = NULL;
            }
            v8::Platform* platform;
            ShellArrayBufferAllocator array_buffer_allocator;
            v8::Isolate::CreateParams create_params;
        };
    #else //>=6
        struct v8init_t
        {
            v8init_t(){
                int argc = 0;
                char* argv[1] = {"h2js"};
                
                v8::V8::InitializeICUDefaultLocation(argv[0]);
                v8::V8::InitializeExternalStartupData(argv[0]);
                platform = v8::platform::CreateDefaultPlatform();
                v8::V8::InitializePlatform(platform);
                v8::V8::Initialize();
                v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
                
                create_params.array_buffer_allocator =
                    v8::ArrayBuffer::Allocator::NewDefaultAllocator();
            }
            ~v8init_t(){
                v8::V8::Dispose();
                v8::V8::ShutdownPlatform();
                delete platform;
                platform = NULL;
                delete create_params.array_buffer_allocator;
                create_params.array_buffer_allocator = NULL;
            }
            v8::Platform* platform;
            v8::Isolate::CreateParams create_params;
        };
    #endif
#endif
struct persistent_lambda_t{
    persistent_lambda_t(v8::Handle<v8::Function> f){
        PERSISTENT_FUNCTION_RESET(jscb, f);//jscb = f;
        // #if V8_MAJOR_VERSION <= 3
            // func = v8::Local<v8::Function>::New(jscb);
        // #else
            // func = jscb;
        // #endif
    }
    ~persistent_lambda_t(){
        if (jscb.IsEmpty() == false){
            ReaseV8Persistent(jscb);
        }
    }
    v8::Handle<v8::Function> getfunc(){
        
        return v8::Local<v8::Function>::New(jscb);
    }
    v8::Persistent<v8::Function> jscb;
    //v8::Local<v8::Function>         func;
};
typedef SharedPtr<persistent_lambda_t>          persistent_lambda_ptr_t;

class FFWorkerJs: public FFWorker, task_processor_i
{
public:
    FFWorkerJs();
    ~FFWorkerJs();
    
    int                     scriptInit(const std::string& js_root);
    
    
    int                     close();
    
    std::string             reload(const std::string& name_);
    //!iterface for python
    void                    js_log(int level, const std::string& mod_, const std::string& content_);


    //!!处理初始化逻辑
    int                     processInit(ConditionVar* var, int* ret, const std::string& js_root);
    
    //**************************************************重载的接口***************************************
    //! 转发client消息
    virtual int onSessionReq(userid_t session_id_, uint16_t cmd_, const std::string& data_);
    //! 处理client 下线
    virtual int onSessionOffline(userid_t session_id);
    //! 处理client 跳转
    virtual int onSessionEnter(userid_t session_id, const std::string& extra_data);
    //! scene 之间的互调用
    virtual std::string onWorkerCall(uint16_t cmd, const std::string& body);
    
    bool call(persistent_lambda_ptr_t& jscb, int argc = 0, v8::Handle<v8::Value>* argv = NULL, std::string* ret = NULL, ScriptArgObjPtr* pRet = NULL);
    
private:
    void                    scriptCleanup();
    void                    scriptRelease();
public:
    bool                            m_enable_call;
    std::string                     m_jspath;
    bool                            m_started;
    
    v8::Isolate*                    m_isolate;
    v8::Persistent<v8::Context>     _global_context;
    v8::Handle<v8::Context>         m_hcontext;
    
    persistent_lambda_ptr_t         m_func_req;
    
    persistent_lambda_ptr_t         m_func_offline;
    
    persistent_lambda_ptr_t         m_func_enter;
    
    persistent_lambda_ptr_t         m_func_rpc;
    
    persistent_lambda_ptr_t         m_func_sync;
    
    SharedPtr<v8init_t>             m_v8init;
};

}

#endif

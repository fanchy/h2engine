#ifndef _FF_FFSCENE_H_
#define _FF_FFSCENE_H_

//#include <assert.h>
#include <string>


#include "base/ffslot.h"
#include "base/fftype.h"
#include "net/codec.h"
#include "rpc/ffrpc.h"
#include "base/arg_helper.h"
#include "server/shared_mem.h"

namespace ff
{
#define EXT_NAME "h2ext"

class ClientData{
public:
    virtual ~ClientData(){}
};

class WorkerClient
{
public:
    WorkerClient(){}
    WorkerClient(const WorkerClient& src):
        session_id(src.session_id),
        session_ip(src.session_ip),
        from_gate(src.from_gate),
        client_data(src.client_data)
    { 
    }
    virtual ~WorkerClient(){}
    template<typename R>
    R* getdata(){
        return (R*)(client_data.get());
    }
    void setdata(ClientData* p){
        SharedPtr<ClientData> sp = p;
        client_data = sp;
    }
public:
    userid_t                        session_id;
    std::string                          session_ip;
    std::string                          from_gate;
    SharedPtr<ClientData>     client_data;
};

typedef bool (*WorkerFunc)();

struct WorkerInitFileInfo{
    const char* strFile;
    int         nLine;
    WorkerFunc  func;
};
#define WORKER_AT_SETUP(f) static int gSetup_##f = FFWorker::regSetupFunc(f, __FILE__, __LINE__)
#define WORKER_AT_EXIT(f)  static int gExit_##f = FFWorker::regExitFunc(f)

class SessionMsgFunctor{
public:
    virtual ~SessionMsgFunctor(){}
    
    virtual void onMsg(userid_t id, const std::string& data) = 0;
};
template<typename T>
struct SessionMsgFunctorUtil;

class FFWorker
{
public:
    static FFWorker* gSingletonWorker;
    static WorkerInitFileInfo gSetupFunc[100];
    static WorkerFunc gExitFunc[100];
    static int regSetupFunc(WorkerFunc f, const char* file, int line);
    static int regExitFunc(WorkerFunc f);
    typedef int (*logic_FFCallBack)(userid_t , uint16_t , const std::string& );
    typedef int (*offline_FFCallBack)(userid_t);
    typedef int (*enter_FFCallBack)(userid_t, const std::string&);
    typedef std::string (*worker_call_FFCallBack)(uint16_t , const std::string& );
    struct callback_info_t
    {
        callback_info_t():
            enter_callback(NULL),
            offline_callback(NULL),
            logic_callback(NULL),
            worker_call_callback(NULL)
        {}
        enter_FFCallBack           enter_callback;
        offline_FFCallBack         offline_callback;
        logic_FFCallBack           logic_callback;
        worker_call_FFCallBack     worker_call_callback;
    };
    
    class session_enter_arg;
    class session_offline_arg;
    class logic_msg_arg;
    class worker_call_msg_arg;
    
    //! 记录session的信息
    struct session_info_t
    {
        std::string gate_name;//! 所在的gate
    };
public:
    FFWorker();
    virtual ~FFWorker();
    virtual int open(const std::string& brokercfg, int worker_index);
    virtual int close();
    //**************************************************操作client 常用的接口******************************************************************************
    int sessionSendMsg(const userid_t& session_id_, uint16_t cmd_, const std::string& data_);
    int gateBroadcastMsg(uint16_t cmd_, const std::string& data_);
    int sessionMulticastMsg(const std::vector<userid_t>& session_id_, uint16_t cmd_, const std::string& data_);
    //! 关闭某个session
    int sessionClose(const userid_t& session_id_);
    //! 切换worker
    int sessionChangeWorker(const userid_t& session_id_, int to_worker_index_, std::string extra_data = "");
    const std::string& getSessionGate(const userid_t& session_id_);
    const std::string& getSessionIp(const userid_t& session_id_);
    
    template<typename F>
    FFWorker& regSessionReq(int cmd, F f){
        typedef typename SessionMsgFunctorUtil<F>::ScriptFunctorImpl FunctorImpl;
        m_functors[cmd] = new FunctorImpl(f);
        return *this;
    }
    template<typename F, typename O>
    FFWorker& regSessionReq(int cmd, F f, O obj){
        typedef typename SessionMsgFunctorUtil<F>::ScriptFunctorImpl FunctorImpl;
        m_functors[cmd] = new FunctorImpl(f, obj);
        return *this;
    }
    //*********************************************************操作client 内部高级接口***********************************************************************
    callback_info_t& callback_info();
    
    //! 发送消息给特定的client
    int sessionSendMsg(const std::string& gate_name, const userid_t& session_id_, uint16_t cmd_, const std::string& data_);
    int sessionKFSendMsg(const std::string& group_name, const std::string& gate_name, const userid_t& session_id_,
                            uint16_t cmd_, const std::string& data_);
    //! 多播
    int sessionMulticastMsg(const std::string& gate_name, const std::vector<userid_t>& session_id_, uint16_t cmd_, const std::string& data_);
    //! 广播 整个gate
    int gateBroadcastMsg(const std::string& gate_name_, uint16_t cmd_, const std::string& data_);
    //! 关闭某个session
    int sessionClose(const std::string& gate_name_, const userid_t& session_id_);
    //! 切换worker
    int sessionChangeWorker(const std::string& gate_name_, const userid_t& session_id_, const std::string& to_worker_, const std::string& extra_data);

    FFRpc& getRpc() { return *m_ffrpc; }
    
    const std::string& getWorkerName() const { return m_logic_name;}
    
    SharedSyncmemMgr& getSharedMem(){ return m_shared_mem_mgr; }
    void regTimer(uint64_t mstimeout_, Task func);
    
    void workerRPC(int workerindex, uint16_t cmd, const std::string& data, FFSlot::FFCallBack* cb);
    void asyncHttp(const std::string& url_, int timeoutsec, FFSlot::FFCallBack* cb);
    std::string syncHttp(const std::string& url_, int timeoutsec);
    
    bool initModule();
    bool cleanupModule();
public:
    //! 转发client消息
    virtual int onSessionReq(userid_t session_id_, uint16_t cmd_, const std::string& data_);
    //! 处理client 下线
    virtual int onSessionOffline(userid_t session_id);
    //! 处理client 跳转
    virtual int onSessionEnter(userid_t session_id, const std::string& extra_data);
    //! scene 之间的互调用
    virtual std::string onWorkerCall(uint16_t cmd, const std::string& body);
   
    userid_t allocUid() { return ++m_allocID; }
    int      getWorkerIndex() const { return m_nWorkerIndex; }
    
protected:
    //! 处理client 进入场景
    int processSessionEnter(ffreq_t<SessionEnterWorker::in_t, SessionEnterWorker::out_t>& req_);
    //! 处理client 下线
    int processSessionOffline(ffreq_t<SessionOffline::in_t, SessionOffline::out_t>& req_);
    //! 转发client消息
    int processSessionReq(ffreq_t<RouteLogicMsg_t::in_t, RouteLogicMsg_t::out_t>& req_);
    //! scene 之间的互调用
    int processWorkerCall(ffreq_t<WorkerCallMsgt::in_t, WorkerCallMsgt::out_t>& req_);
    
protected:
    int                                         m_nWorkerIndex;//! worker index num
    userid_t                                    m_allocID;
    std::string                                 m_logic_name;
    SharedPtr<FFRpc>                            m_ffrpc;
    callback_info_t                             m_callback_info;
    
    std::map<userid_t, WorkerClient>            m_worker_client;
    SharedSyncmemMgr                            m_shared_mem_mgr;
    std::map<int, SessionMsgFunctor*>           m_functors;
};

class FFWorkerMgr
{
public:
    FFWorker* get(const std::string& name_)
    {
        std::map<std::string, FFWorker*>::iterator it = m_all_worker.find(name_);
        if (it != m_all_worker.end())
        {
            return it->second;
        }
        return NULL;
    }
    FFWorker* get_any()
    {
        std::map<std::string, FFWorker*>::iterator it = m_all_worker.begin();
        if (it != m_all_worker.end())
        {
            return it->second;
        }
        return NULL;
    }
    FFWorker* get_like(const std::string& s)
    {
        std::map<std::string, FFWorker*>::iterator it = m_all_worker.begin();
        if (it != m_all_worker.end())
        {
            if (it->first.find(s) != std::string::npos)
                return it->second;
        }
        return NULL;
    }
    void       add(const std::string& name_, FFWorker* p)
    {
        m_all_worker[name_] = p;
    }
    void       del(const std::string& name_)
    {
        m_all_worker.erase(name_);
    }
    
private:
    std::map<std::string, FFWorker*>     m_all_worker;
};
#define FFWORKER_SINGLETON (*FFWorker::gSingletonWorker)

class FFWorker::session_enter_arg: public FFSlot::CallBackArg
{
public:
    session_enter_arg(const std::string& session_ip_, const std::string& gate_, const userid_t& s_,
                      const std::string& from_, const std::string& to_, const std::string& data_):
        session_ip(session_ip_),
        gate_name(gate_),
        session_id(s_),
        from_worker(from_),
        to_worker(to_),
        extra_data(data_)
    {}
    virtual int type()
    {
        return TYPEID(session_enter_arg);
    }
    std::string    session_ip;
    std::string    gate_name;
    userid_t    session_id;//! 包含用户id
    std::string    from_worker;//! 从哪个scene跳转过来,若是第一次上线，from_worker为空
    std::string    to_worker;//! 跳到哪个scene上面去,若是下线，to_worker为空
    std::string    extra_data;//! 附带数据
};
class FFWorker::session_offline_arg: public FFSlot::CallBackArg
{
public:
    session_offline_arg(const userid_t& s_):
        session_id(s_)
    {}
    virtual int type()
    {
        return TYPEID(session_offline_arg);
    }
    userid_t          session_id;
};
class FFWorker::logic_msg_arg: public FFSlot::CallBackArg
{
public:
    logic_msg_arg(const userid_t& s_, uint16_t cmd_, const std::string& t_):
        session_id(s_),
        cmd(cmd_),
        body(t_)
    {}
    virtual int type()
    {
        return TYPEID(logic_msg_arg);
    }
    userid_t                session_id;
    uint16_t                cmd;
    std::string             body;
};

class FFWorker::worker_call_msg_arg: public FFSlot::CallBackArg
{
public:
    worker_call_msg_arg(uint16_t cmd_, const std::string& t_, std::string& err_, std::string& msg_type_, std::string& ret_):
        cmd(cmd_),
        body(t_),
        err(err_),
        msg_type(msg_type_),
        ret(ret_)
    {}
    virtual int type()
    {
        return TYPEID(worker_call_msg_arg);
    }
    uint16_t             cmd;
    const std::string&   body;
    
    std::string&         err;
    std::string&         msg_type;
    std::string&         ret;
};
template<>
struct SessionMsgFunctorUtil<void (*)(userid_t, const std::string&)>{
    typedef void (*DestFunc)(userid_t, const std::string&);
    class ScriptFunctorImpl: public SessionMsgFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void onMsg(userid_t id, const std::string& data){
            if (destFunc){
                (*destFunc)(id, data);
            }
        }
        DestFunc destFunc;
    };
};
template<typename FUNC_CLASS_TYPE>
struct SessionMsgFunctorUtil<void (FUNC_CLASS_TYPE::*)(userid_t, const std::string&)>{
    typedef void (FUNC_CLASS_TYPE::*DestFunc)(userid_t, const std::string&);
    class ScriptFunctorImpl: public SessionMsgFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void onMsg(userid_t id, const std::string& data){
            if (destFunc){
                (obj->*destFunc)(id, data);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

}


#endif

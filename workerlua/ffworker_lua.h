#ifndef _FF_FFWORKER_LUA_H_
#define _FF_FFWORKER_LUA_H_

//#include <assert.h>
//#include <string>


#include "base/log.h"
#include "server/db_mgr.h"
#include "server/fftask_processor.h"
#include "server/ffworker.h"

namespace ff
{
#define FFWORKER_LUA "FFWORKER_LUA"
struct luaops_t;
class FFWorkerLua: public FFWorker, task_processor_i
{
public:
    FFWorkerLua();
    ~FFWorkerLua();
    
    luaops_t&                getFFlua(){ return *m_fflua; }
    
    int                     scriptInit(const std::string& py_root);
    void                    scriptCleanup();
    
    int                     close();
    
    std::string             reload(const std::string& name_);
    //!iterface for python
    void                    pylog(int level, const std::string& mod_, const std::string& content_);

    //!!处理初始化逻辑
    int                     processInit(Mutex* mutex, ConditionVar* var, int* ret);
    
    //**************************************************重载的接口***************************************
    //! 转发client消息
    virtual int onSessionReq(userid_t session_id_, uint16_t cmd_, const std::string& data_);
    //! 处理client 下线
    virtual int onSessionOffline(userid_t session_id);
    //! 处理client 跳转
    virtual int onSessionEnter(userid_t session_id, const std::string& extra_data);
    //! scene 之间的互调用
    virtual std::string onWorkerCall(uint16_t cmd, const std::string& body);
    
public:
    bool                    m_enable_call;
    bool                    m_started;
    luaops_t*               m_fflua;
    std::string             m_ext_name;
};

}

#endif

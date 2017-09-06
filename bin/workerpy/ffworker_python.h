#ifndef _FF_FFWORKER_PYTHON_H_
#define _FF_FFWORKER_PYTHON_H_

//#include <assert.h>
//#include <string>


#include "base/log.h"
#include "server/db_mgr.h"
#include "server/fftask_processor.h"
#include "server/ffworker.h"

class ffpython_t;
namespace ff
{
#define FFWORKER_PYTHON "FFWORKER_PYTHON"

class FFWorkerPython: public FFWorker, task_processor_i
{
public:
    FFWorkerPython();
    ~FFWorkerPython();
    
    ffpython_t&             get_ffpython(){ return *m_ffpython; }
    
    int                     py_init(const std::string& py_root);
    void                    py_cleanup();
    
    int                     close();
    
    std::string             reload(const std::string& name_);
    //!iterface for python
    void                    pylog(int level, const std::string& mod_, const std::string& content_);

    //!!处理初始化逻辑
    int                     process_init(ConditionVar* var, int* ret);
    
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
    ffpython_t*             m_ffpython;
    std::string             m_ext_name;
};

}

#endif

#ifndef _FF_DB_MGR_H_
#define _FF_DB_MGR_H_

#include <assert.h>
#include <string>
#include <vector>
#include <map>

#include "base/fftype.h"
#include "base/log.h"
#include "base/smart_ptr.h"
#include "db/ffdb.h"
#include "base/lock.h"
#include "base/ffslot.h"
#include "base/singleton.h"

namespace ff
{

#define DB_MGR_LOG "DB_MGR"
class QueryDBResult: public FFSlot::CallBackArg
{
public:
    QueryDBResult():
        ok(false)
    {}
    virtual ~QueryDBResult(){}
    virtual int type()
    {
        return TYPEID(QueryDBResult);
    }
    void clear()
    {
        ok = false;
        dataResult.clear();
        fieldNames.clear();
    }
    bool                                    ok;
    std::vector<std::vector<std::string> >  dataResult;
    std::vector<std::string>                fieldNames;
    std::string                             errinfo;
    int                                     affectedRows;
};
template <typename T>
struct DbCallBack: public FFSlot::FFCallBack
{
    DbCallBack(T pFuncArg, TaskQueue* ptq):pFunc(pFuncArg), tq(ptq){}
    virtual void exe(FFSlot::CallBackArg* args_)
    {
        //LOGINFO((DB_MGR_LOG, "DbMgr::DbCallBack tq:%d", long(tq)));
        //sleep(10);
        QueryDBResult* data = (QueryDBResult*)args_;
        if (!tq){
            pFunc(*data);
        }
        else{
            tq->post(TaskBinder::gen(&DbCallBack<T>::tqCall, pFunc, *data));
        }
    }
    static void tqCall(T func, QueryDBResult& data){
        func(data);
    }
    virtual FFSlot::FFCallBack* fork() { return new DbCallBack<T>(pFunc, tq); }
    T pFunc;
    TaskQueue* tq;
};
#define DB_THREAD_NUM 10
#define DB_DEFAULT_NAME "Default"
#define DB_DEFAULT_NAME_1 "Default#1"
typedef SharedPtr<FFSlot::FFCallBack> FFSlotCallBackPtr;
class DbMgr
{
public:
    struct DBConnectionInfo
    {
        DBConnectionInfo():result_flag(-1),cond(mutex)
        {
        }
        DBConnectionInfo(const DBConnectionInfo& src_):
            tq(src_.tq),
            db(src_.db),
            result_flag(-1),
            cond(mutex)
        {
        }
        void signal();
        int  wait();
        
        SharedPtr<TaskQueue>            tq;
        SharedPtr<FFDb>                 db;
        QueryDBResult                   ret;
        std::string                     name;

        //!条件变量，用于同步查询的接口
        volatile int                    result_flag;
        Mutex                           mutex;
        ConditionVar                    cond;
        std::string                     host_cfg;
    };
    typedef QueryDBResult queryDBResult_t;
public:
    DbMgr();
    ~DbMgr();
    int start();
    int stop();

    long connectDB(const std::string& host_, const std::string& name);

    template<typename T>
    void asyncQueryModId(long mod, const std::string& sql_, T& func, TaskQueue* tq = NULL){
        if (!tq){
            tq = m_tqWorkerDefault;
        }
        char buff[256] = {0};
        int nMod = (m_defaultDbNum != 0)? m_defaultDbNum: 1;
        ::snprintf(buff, sizeof(buff), "%s#%ld", DB_DEFAULT_NAME, mod % nMod + 1);
        asyncQueryByName(buff, sql_, func, tq);
    }

    void asyncQueryModId(long mod, const std::string& sql_);
    int  query(const std::string& sql_, std::vector<std::vector<std::string> >* ret_data_ = NULL,
                     std::string* errinfo = NULL, int* affectedRows_ = NULL, std::vector<std::string>* col_ = NULL);

    template<typename T>
    void asyncQueryByName(const std::string& strName, const std::string& sql_, T& func, TaskQueue* tq){
        DBConnectionInfo* varDbConnection = NULL;
        {
            LockGuard lock(m_mutex);
            varDbConnection = getConnectionByName(strName);
        }
        //LOGINFO(("DB_MGR", "DbMgr::asyncQueryByName name<%s>, sql<%s> varDbConnection:%p", strName, sql_, long(varDbConnection)));
        if (NULL == varDbConnection)
        {
            QueryDBResult result;
            result.errinfo = "no db cfg";
            if (tq){
                tq->post(TaskBinder::gen(&DbCallBack<T>::tqCall, func, result));
            }
            else{
                func(result);
            }
            
            return;
        }
        else
        {
            varDbConnection->tq->post(TaskBinder::gen(&DbMgr::queryDBImpl, this, varDbConnection, sql_, new DbCallBack<T>(func, tq)));
        }
    }
    void asyncQueryByName(const std::string& strName, const std::string& sql_);
    int  queryByName(const std::string& strName, const std::string& sql_, 
                     std::vector<std::vector<std::string> >* ret_data_ = NULL,
                     std::string* errinfo = NULL, int* affectedRows_ = NULL, std::vector<std::string>* col_ = NULL);
    uint64_t allocId(int nType);
    void setDefaultTaskQueue(TaskQueue* tq){ m_tqWorkerDefault = tq; }
private:
    void queryDBImpl(DBConnectionInfo* db_connection_info_, const std::string& sql_, FFSlotCallBackPtr callback_);
    void syncQueryDBImpl(DBConnectionInfo* db_connection_info_, const std::string& sql_, QueryDBResult* result);
    DBConnectionInfo* getConnectionById(long cid){
        std::map<long/*dbid*/, DBConnectionInfo>::iterator it2 = m_db_connection.find(cid);
        if (it2 != m_db_connection.end()){
            return &(it2->second);
        }
        return NULL;
    }
    DBConnectionInfo* getConnectionByName(const std::string& name){
        std::map<std::string, DBConnectionInfo*>::iterator it = m_name2connection.find(name);
        if (it != m_name2connection.end()){
            return it->second;
        }
        return NULL;
    }
    int getNumLike(const std::string& strName);
private:
    long                                                m_db_index;
    int                                                 m_defaultDbNum;
    std::vector<SharedPtr<TaskQueue> >                  m_tq;
    Mutex                                               m_mutex;
    std::map<long/*dbid*/, DBConnectionInfo>            m_db_connection;
    Thread                                              m_thread;
    std::map<std::string, DBConnectionInfo*>            m_name2connection;
    TaskQueue*                                          m_tqWorkerDefault;
};
#define DB_MGR Singleton<DbMgr>::instance()



}

#endif

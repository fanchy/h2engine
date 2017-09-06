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

class DbMgr
{
public:
    struct db_connection_info_t;
    class queryDBResult_t;
public:
    DbMgr();
    ~DbMgr();
    int start();
    int stop();

    long connectDB(const std::string& host_, const std::string& groupName);
    void queryDB(long db_id_,const std::string& sql_, FFSlot::FFCallBack* callback_);
    int  syncQueryDB(long db_id_,const std::string& sql_, std::vector<std::vector<std::string> >& ret_data_,
                     std::vector<std::string>& col_, std::string& errinfo, int& affectedRows_);

    void queryDBGroupMod(const std::string& strGroupName, long mod, const std::string& sql_, FFSlot::FFCallBack* callback_);
    int  syncQueryDBGroupMod(const std::string& strGroupName, long mod, const std::string& sql_, 
                     std::vector<std::vector<std::string> >& ret_data_,
                     std::vector<std::string>& col_, std::string& errinfo, int& affectedRows_);
private:
    void queryDBImpl(db_connection_info_t* db_connection_info_, const std::string& sql_, FFSlot::FFCallBack* callback_);
    void syncQueryDBImpl(db_connection_info_t* db_connection_info_, const std::string& sql_, std::vector<std::vector<std::string> >* pRet, std::vector<std::string>* col_, std::string* errinfo, int* affectedRows_);
private:
    long                                                m_db_index;
    std::vector<SharedPtr<TaskQueue> >                  m_tq;
    Mutex                                               m_mutex;
    std::map<long/*dbid*/, db_connection_info_t>        m_db_connection;
    Thread                                              m_thread;
    std::map<std::string, std::vector<long> >           m_group2connection;
};
#define DB_MGR_OBJ Singleton<DbMgr>::instance()

class DbMgr::queryDBResult_t: public FFSlot::CallBackArg
{
public:
    queryDBResult_t():
        ok(false)
    {}
    virtual int type()
    {
        return TYPEID(queryDBResult_t);
    }
    void clear()
    {
        ok = false;
        result_data.clear();
        col_names.clear();
    }
    bool                                    ok;
    std::vector<std::vector<std::string> >  result_data;
    std::vector<std::string>                col_names;
    std::string                             errinfo;
    int                                     affectedRows;
};

struct DbMgr::db_connection_info_t
{
    db_connection_info_t():result_flag(-1),cond(mutex)
    {
    }
    db_connection_info_t(const db_connection_info_t& src_):
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
    queryDBResult_t                 ret;
    std::string                     name;

    //!条件变量，用于同步查询的接口
    volatile int                    result_flag;
    Mutex                           mutex;
    ConditionVar                    cond;
    std::string                     host_cfg;
};

}

#endif

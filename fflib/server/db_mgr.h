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
#include "base/singleton.h"

namespace ff
{

#define DB_MGR_LOG "DB_MGR"
class QueryDBResult
{
public:
    QueryDBResult():affectedRows(0){}
    virtual ~QueryDBResult(){}

    void clear()
    {
        dataResult.clear();
        fieldNames.clear();
        errinfo.clear();
        affectedRows = 0;
    }
    std::vector<std::vector<std::string> >  dataResult;
    std::vector<std::string>                fieldNames;
    std::string                             errinfo;
    int                                     affectedRows;
};

class DbMgr
{
public:
    struct DBConnectionInfo
    {
        SharedPtr<TaskQueue>            tq;
        SharedPtr<FFDb>                 db;
    };
    //typedef QueryDBResult queryDBResult_t;
public:
    DbMgr();
    ~DbMgr();
    static DbMgr& instance() { return Singleton<DbMgr>::instance(); }
    int initDBPool(const std::string& host_, int nNums);
    int cleanup();
    int query(const std::string& sql){QueryDBResult ret;return query(sql, ret);};
    int query(const std::string& sql, QueryDBResult& ret);
    int asyncQuery(int64_t modid, const std::string& sql, Function<void(QueryDBResult&)> callback = NULL, TaskQueue* callbackTQ = NULL);

    int queryByCfg(const std::string& host_, const std::string& sql, QueryDBResult& ret);
    int asyncQueryByCfg(const std::string& host_, int64_t modid, const std::string& sql, Function<void(QueryDBResult&)> callback = NULL, TaskQueue* callbackTQ = NULL);
private:
    int doQuery(SharedPtr<FFDb> db, const std::string& sql, Function<void(QueryDBResult&)> callback, TaskQueue* callbackTQ);

private:
    std::vector<SharedPtr<DBConnectionInfo> >           m_dbPool;
    SharedPtr<FFDb>                                     m_dbForSync;
};




}

#endif

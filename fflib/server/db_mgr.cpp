
#include "server/db_mgr.h"
#include "base/perf_monitor.h"
using namespace ff;
using namespace std;



DbMgr::DbMgr()
{
}
DbMgr::~DbMgr()
{
    cleanup();
}

int DbMgr::cleanup()
{
    if (m_dbPool.empty()){
        return 0;
    }
    LOGINFO((DB_MGR_LOG, "DbMgr::stop begin..."));
    for (size_t i = 0; i < m_dbPool.size(); ++i)
    {
        m_dbPool[i]->tq->close();
    }
    m_dbPool.clear();
    LOGINFO((DB_MGR_LOG, "DbMgr::stop end"));
    return 0;
}

int DbMgr::initDBPool(const string& host_, int nNums)
{
    m_dbForSync = new FFDb();
    if (m_dbForSync->connect(host_))
    {
        LOGERROR((DB_MGR_LOG, "DbMgr::connectDB failed<%s>", m_dbForSync->errorMsg()));
        m_dbForSync = NULL;
        return -1;
    }
    for (int i = 0; i < nNums; ++i)
    {
        SharedPtr<FFDb> db(new FFDb());
        if (db->connect(host_))
        {
            LOGERROR((DB_MGR_LOG, "DbMgr::connectDB failed<%s>", db->errorMsg()));
            return -1;
        }
        
        m_dbPool.push_back(new DBConnectionInfo());
        DBConnectionInfo& dbinfo = *(m_dbPool[i]);
        dbinfo.db = db;
        dbinfo.tq = new TaskQueue();
    }
    LOGINFO((DB_MGR_LOG, "DbMgr::connectDB host<%s>,num<%d>", host_, nNums));
    return 0;
}
int DbMgr::query(const std::string& sql, QueryDBResult& ret)
{
    if (!m_dbForSync){
        ret.errinfo = "db not initial";
        return -1;
    }
    if (0 == m_dbForSync->exeSql(sql, ret.dataResult, ret.fieldNames))
    {
        ret.affectedRows = m_dbForSync->affectedRows();
    }
    else{
        ret.errinfo = m_dbForSync->errorMsg();
    }
    return 0;
}
int DbMgr::asyncQuery(int64_t modid, const std::string& sql, Function<void(QueryDBResult&)> callback, TaskQueue* callbackTQ)
{
    if (m_dbPool.empty()){
        QueryDBResult ret;
        ret.errinfo = "db not initial";
        callback(ret);
        return -1;
    }
    DBConnectionInfo& dbinfo = *(m_dbPool[modid % m_dbPool.size()]);
    dbinfo.tq->post(funcbind(&DbMgr::doQuery, this, dbinfo.db, sql, callback, callbackTQ));
    return 0;
}
static void threadSafeCall(Function<void(QueryDBResult&)> callback, QueryDBResult ret){
    if (callback){
        callback(ret);
    }
}
int DbMgr::doQuery(SharedPtr<FFDb> db, const std::string& sql, Function<void(QueryDBResult&)> callback, TaskQueue* callbackTQ)
{
    QueryDBResult ret;
    if (0 == db->exeSql(sql, ret.dataResult, ret.fieldNames))
    {
        ret.affectedRows = db->affectedRows();
    }
    else{
        ret.errinfo = db->errorMsg();
    }
    if (!callback){
        return 0;
    }
    if (!callbackTQ){
        callback(ret);
    }
    callbackTQ->post(funcbind(&threadSafeCall, callback, ret));
    return 0;
}
int DbMgr::queryByCfg(const std::string& host_, const std::string& sql, QueryDBResult& ret)
{
    SharedPtr<FFDb> db = new FFDb();
    if (db->connect(host_))
    {
        LOGERROR((DB_MGR_LOG, "DbMgr::connectDB failed<%s>", db->errorMsg()));
        db = NULL;
        return 0;
    }
    if (0 == db->exeSql(sql, ret.dataResult, ret.fieldNames))
    {
        ret.affectedRows = db->affectedRows();
    }
    else{
        ret.errinfo = db->errorMsg();
    }
    return 0;
}

int DbMgr::asyncQueryByCfg(const std::string& host_, int64_t modid, const std::string& sql, Function<void(QueryDBResult&)> callback, TaskQueue* callbackTQ)
{
    if (m_dbPool.empty()){
        QueryDBResult ret;
        ret.errinfo = "db not initial";
        callback(ret);
        return -1;
    }
    DBConnectionInfo& dbinfo = *(m_dbPool[modid % m_dbPool.size()]);

    SharedPtr<FFDb> db = new FFDb();
    if (db->connect(host_))
    {
        LOGERROR((DB_MGR_LOG, "DbMgr::connectDB failed<%s>", db->errorMsg()));
        db = NULL;
        return 0;
    }

    dbinfo.tq->post(funcbind(&DbMgr::doQuery, this, db, sql, callback, callbackTQ));
    return 0;
}

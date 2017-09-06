
#include "server/db_mgr.h"
#include "base/performance_daemon.h"
using namespace ff;
using namespace std;

#define DB_MGR "DB_MGR"

DbMgr::DbMgr():
    m_db_index(0)
{
}
DbMgr::~DbMgr()
{
}

int DbMgr::start()
{
    for (size_t i = 0; i < 10; ++i)
    {
        m_tq.push_back(new TaskQueue());
        m_thread.create_thread(TaskBinder::gen(&TaskQueue::run, (m_tq[i]).get()), 1);
    }
    return 0;
}
int DbMgr::stop()
{
    LOGINFO((DB_MGR, "DbMgr::stop begin..."));
    for (size_t i = 0; i < m_tq.size(); ++i)
    {
        m_tq[i]->close();
    }
    m_thread.join();
    LOGINFO((DB_MGR, "DbMgr::stop end"));
    return 0;
}


long DbMgr::connectDB(const string& host_, const std::string& groupName)
{
    SharedPtr<FFDb> db(new FFDb());
    if (db->connect(host_))
    {
        LOGERROR((DB_MGR, "DbMgr::connectDB failed<%s>", db->errorMsg()));
        return 0;
    }
    
    long db_id = 0;
    {
        LockGuard lock(m_mutex);
        static long autoid = 0;
        db_id = ++autoid;
        
        db_connection_info_t& db_connection_info = m_db_connection[db_id];
        db_connection_info.db = db;
        db_connection_info.tq = m_tq[m_db_index++ % m_tq.size()];
        db_connection_info.host_cfg = host_;
        
        m_group2connection[groupName].push_back(db_id);
    }
    return db_id;
}

void DbMgr::queryDB(long db_id_,const string& sql_, FFSlot::FFCallBack* callback_)
{
    db_connection_info_t* db_connection_info = NULL;
    {
        LockGuard lock(m_mutex);
        db_connection_info = &(m_db_connection[db_id_]);
    }
    if (NULL == db_connection_info || !db_connection_info->tq)
    {
        queryDBResult_t ret;
        callback_->exe(&ret);
        delete callback_;
        return;
    }
    else
    {
        db_connection_info->tq->produce(TaskBinder::gen(&DbMgr::queryDBImpl, this, db_connection_info, sql_, callback_));
    }
}

void DbMgr::queryDBGroupMod(const std::string& strGroupName, long mod, const std::string& sql_, FFSlot::FFCallBack* callback_){
    db_connection_info_t* db_connection_info = NULL;
    {
        LockGuard lock(m_mutex);
        std::vector<long>& groupConnections = m_group2connection[strGroupName];
        if (groupConnections.empty()){
            LOGERROR((DB_MGR, "DbMgr::queryDBGroupMod failed emptyGroup<%s>, while sql<%s>", strGroupName, sql_));
            return;
        }
        long db_id_ = groupConnections[mod % groupConnections.size()];
        db_connection_info = &(m_db_connection[db_id_]);
    }
    if (NULL == db_connection_info || !db_connection_info->tq)
    {
        queryDBResult_t ret;
        callback_->exe(&ret);
        delete callback_;
        return;
    }
    else
    {
        db_connection_info->tq->produce(TaskBinder::gen(&DbMgr::queryDBImpl, this, db_connection_info, sql_, callback_));
    }
}
void DbMgr::queryDBImpl(db_connection_info_t* db_connection_info_, const string& sql_, FFSlot::FFCallBack* callback_)
{
    LOGTRACE((DB_MGR, "DbMgr::queryDBImpl sql=%s", sql_));
    AUTO_PERF();
    db_connection_info_->ret.clear();
    if (0 == db_connection_info_->db->exeSql(sql_, db_connection_info_->ret.result_data, db_connection_info_->ret.col_names))
    {
        db_connection_info_->ret.ok = true;
        db_connection_info_->ret.affectedRows = db_connection_info_->db->affectedRows();
    }
    else
    {
        db_connection_info_->ret.errinfo = db_connection_info_->db->errorMsg();
        LOGERROR((DB_MGR, "DbMgr::queryDB failed<%s>, while sql<%s>", db_connection_info_->db->errorMsg(), sql_));
    }
    if (callback_)
    {
        callback_->exe(&(db_connection_info_->ret));
        delete callback_;
    }
}

int DbMgr::syncQueryDB(long db_id_,const string& sql_, vector<vector<string> >& ret_data_, vector<string>& col_, string& errinfo, int& affectedRows_)
{
    AUTO_PERF();
    db_connection_info_t* db_connection_info = NULL;
    {
        LockGuard lock(m_mutex);
        map<long/*dbid*/, db_connection_info_t>::iterator it = m_db_connection.find(db_id_);
        if (it == m_db_connection.end())
        {
        	return -1;
		}
        db_connection_info = &(it->second);
    }
    if (NULL == db_connection_info || !db_connection_info->tq)
    {
        return -1;
    }
    
    db_connection_info->result_flag = 0;//!标记开始同步查询
    db_connection_info->tq->produce(TaskBinder::gen(&DbMgr::syncQueryDBImpl, this, db_connection_info, sql_, &ret_data_, &col_, &errinfo, &affectedRows_));
    db_connection_info->wait();
    return 0;
}
int  DbMgr::syncQueryDBGroupMod(const std::string& strGroupName, long mod, const std::string& sql_, 
                     std::vector<std::vector<std::string> >& ret_data_,
                     std::vector<std::string>& col_, std::string& errinfo, int& affectedRows_)
{
    AUTO_PERF();
    db_connection_info_t* db_connection_info = NULL;
    {
        LockGuard lock(m_mutex);
        std::vector<long>& groupConnections = m_group2connection[strGroupName];
        if (groupConnections.empty()){
            LOGERROR((DB_MGR, "DbMgr::syncQueryDBGroupMod failed emptyGroup<%s>, while sql<%s>", strGroupName, sql_));
            return -1;
        }
        long db_id_ = groupConnections[mod % groupConnections.size()];
        
        map<long/*dbid*/, db_connection_info_t>::iterator it = m_db_connection.find(db_id_);
        if (it == m_db_connection.end())
        {
        	return -1;
		}
        db_connection_info = &(it->second);
    }
    if (NULL == db_connection_info || !db_connection_info->tq)
    {
        return -1;
    }
    
    db_connection_info->result_flag = 0;//!标记开始同步查询
    db_connection_info->tq->produce(TaskBinder::gen(&DbMgr::syncQueryDBImpl, this, db_connection_info, sql_, &ret_data_, &col_, &errinfo, &affectedRows_));
    db_connection_info->wait();
    return 0;
}
void DbMgr::syncQueryDBImpl(db_connection_info_t* db_connection_info, const string& sql_, vector<vector<string> >* pRet, vector<string>* col_, string* errinfo, int* affectedRows_)
{
    LOGTRACE((DB_MGR, "DbMgr::syncQueryDBImpl sql=%s", sql_));
    db_connection_info->ret.clear();
    if (0 == db_connection_info->db->exeSql(sql_, *pRet, *col_))
    {
        *affectedRows_ = db_connection_info->db->affectedRows();
    }
    else
    {
        *errinfo = db_connection_info->db->errorMsg();
        LOGERROR((DB_MGR, "DbMgr::syncQueryDB failed<%s>, while sql<%s>", db_connection_info->db->errorMsg(), sql_));
    }
    db_connection_info->signal();
}
void DbMgr::db_connection_info_t::signal()
{
    LockGuard lock(mutex);
    result_flag = 1;
    cond.signal();
}

int DbMgr::db_connection_info_t::wait()
{
    LockGuard lock(mutex);
    if (1 == result_flag)
    {
        return 0;
    }
    
    cond.wait();
    result_flag = 0;
    return 0;
}


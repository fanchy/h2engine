
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
bool initIdGenerator(){
/*
sql = """
create table id_generator
(
  AUTO_INC_ID bigint not null,
  TYPE int not null,
  SERVER_ID int not null,
  RUNING_FLAG int not null,
  primary key(TYPE, SERVER_ID)
)
"""
class idgen_t:
    def __init__(self, db_host_, type_id_ = 0, server_id_ = 0):
        self.type_id = type_id_
        self.server_id = server_id_
        self.auto_inc_id = 0
        self.db_host = db_host_
        self.db      = None
        self.saving_flag = False
        self.runing_flag = 0
    def init(self):
        self.db = ffext.ffdb_create(self.db_host)
        ret = self.db.sync_query("SELECT `AUTO_INC_ID`, `RUNING_FLAG` FROM `id_generator` WHERE `TYPE` = '%d' AND `SERVER_ID` = '%d'" % (self.type_id, self.server_id))
        #print(ret.flag, ret.result, ret.column)
        if len(ret.result) == 0:
            #数据库中还没有这一行，插入
            self.db.sync_query("INSERT INTO `id_generator` SET `AUTO_INC_ID` = '0',`TYPE` = '%d', `SERVER_ID` = '%d', `RUNING_FLAG` = '1' " % (self.type_id, self.server_id))
            return True
        else:
            self.auto_inc_id = int(ret.result[0][0])
            self.runing_flag = int(ret.result[0][1])
            if self.runing_flag != 0:
                self.auto_inc_id += 10000
                ffext.ERROR('last idgen shut down not ok, inc 10000')
            self.db.sync_query("UPDATE `id_generator` SET `RUNING_FLAG` = '1' WHERE `TYPE` = '%d' AND `SERVER_ID` = '%d'" % (self.type_id, self.server_id))
        #if self.auto_inc_id < 65535:
        #    self.auto_inc_id = 65535
        return True
    def cleanup(self):
        db = ffext.ffdb_create(self.db_host)
        now_val = self.auto_inc_id
        db.sync_query("UPDATE `id_generator` SET `AUTO_INC_ID` = '%d', `RUNING_FLAG` = '0' WHERE `TYPE` = '%d' AND `SERVER_ID` = '%d'" % (now_val, self.type_id, self.server_id))
        return True
    def gen_id(self):
        self.auto_inc_id += 1
        self.update_id()
        low16 = self.auto_inc_id & 0xFFFF
        high  = (self.auto_inc_id >> 16) << 32
        return high | (self.server_id << 16)| low16
    def dump_id(self, id_):
        low16 = id_ & 0xFFFF
        high  = id_ >> 32
        return high << 16 | low16
    def update_id(self):
        if True == self.saving_flag:
            return
        self.saving_flag = True
        now_val = self.auto_inc_id
        def cb(ret):
            #print(ret.flag, ret.result, ret.column)
            self.saving_flag = False
            if now_val < self.auto_inc_id:
                self.update_id()
        self.db.query("UPDATE `id_generator` SET `AUTO_INC_ID` = '%d' WHERE `TYPE` = '%d' AND `SERVER_ID` = '%d' AND `AUTO_INC_ID` < '%d'" % (now_val, self.type_id, self.server_id, now_val), cb)
        return
*/
   return true;
}

uint64_t DbMgr::allocId(int nType)
{
    uint64_t ret = 0;
    return ret;
}




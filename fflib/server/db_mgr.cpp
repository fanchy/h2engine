
#include "server/db_mgr.h"
#include "base/perf_monitor.h"
using namespace ff;
using namespace std;



DbMgr::DbMgr():
    m_db_index(0), m_defaultDbNum(0), m_tqWorkerDefault(NULL)
{
}
DbMgr::~DbMgr()
{
}

int DbMgr::start()
{
    for (size_t i = 0; i < DB_THREAD_NUM; ++i)
    {
        m_tq.push_back(new TaskQueue());
    }
    return 0;
}
int DbMgr::stop()
{
    LOGINFO((DB_MGR_LOG, "DbMgr::stop begin..."));
    for (size_t i = 0; i < m_tq.size(); ++i)
    {
        m_tq[i]->close();
    }

    LOGINFO((DB_MGR_LOG, "DbMgr::stop end"));
    return 0;
}


long DbMgr::connectDB(const string& host_, const std::string& name)
{
    SharedPtr<FFDb> db(new FFDb());
    if (db->connect(host_))
    {
        LOGERROR((DB_MGR_LOG, "DbMgr::connectDB failed<%s>", db->errorMsg()));
        return 0;
    }

    long db_id = 0;
    {
        LockGuard lock(m_mutex);
        static long autoid = 0;
        db_id = ++autoid;

        DBConnectionInfo& varDbConnection = m_db_connection[db_id];
        varDbConnection.db = db;
        varDbConnection.tq = m_tq[m_db_index++ % m_tq.size()];
        varDbConnection.host_cfg = host_;

        if (name == DB_DEFAULT_NAME){
            ++m_defaultDbNum;
            char buff[256] = {0};
            ::snprintf(buff, sizeof(buff), "%s#%d", DB_DEFAULT_NAME, m_defaultDbNum);
            m_name2connection[buff] = &varDbConnection;
        }
        else{
            m_name2connection[name] = &varDbConnection;
        }
    }
    LOGINFO((DB_MGR_LOG, "DbMgr::connectDB host<%s>,groupName<%s>", host_, name));
    return db_id;
}

void DbMgr::asyncQueryModId(long mod, const std::string& sql_){
    char buff[256] = {0};
    int nMod = (m_defaultDbNum != 0)? m_defaultDbNum: 1;
    ::snprintf(buff, sizeof(buff), "%s#%ld", DB_DEFAULT_NAME, mod % nMod + 1);
    asyncQueryByName(buff, sql_);
}
int  DbMgr::query(const std::string& sql_, std::vector<std::vector<std::string> >* ret_data_,
                     std::string* errinfo, int* affectedRows_, std::vector<std::string>* col_){
    return queryByName(DB_DEFAULT_NAME_1, sql_, ret_data_, errinfo, affectedRows_, col_);
}
void DbMgr::asyncQueryByName(const std::string& strName, const std::string& sql_){
    DBConnectionInfo* varDbConnection = NULL;
    {
        LockGuard lock(m_mutex);
        varDbConnection = getConnectionByName(strName);
    }
    if (NULL == varDbConnection)
    {
        return;
    }
    else
    {
        FFSlot::FFCallBack* callback_ = NULL;
        varDbConnection->tq->post(funcbind(&DbMgr::queryDBImpl, this, varDbConnection, sql_, callback_));
    }
}
void DbMgr::queryDBImpl(DBConnectionInfo* varDbConnection_, const string& sql_, FFSlotCallBackPtr callback_)
{
    AUTO_PERF();
    if (!varDbConnection_){
        if (callback_)
        {
            QueryDBResult result;
            callback_->exe(&(result));
        }
        return;
    }
    varDbConnection_->ret.clear();
    if (0 == varDbConnection_->db->exeSql(sql_, varDbConnection_->ret.dataResult, varDbConnection_->ret.fieldNames))
    {
        varDbConnection_->ret.ok = true;
        varDbConnection_->ret.affectedRows = varDbConnection_->db->affectedRows();
    }
    else
    {
        varDbConnection_->ret.errinfo = varDbConnection_->db->errorMsg();
        LOGERROR((DB_MGR_LOG, "DbMgr::queryDB failed<%s>, while sql<%s>", varDbConnection_->db->errorMsg(), sql_));
    }
    if (callback_)
    {
        callback_->exe(&(varDbConnection_->ret));
    }
    LOGINFO((DB_MGR_LOG, "DbMgr::queryDBImpl sql=%s end", sql_));
}

int  DbMgr::queryByName(const std::string& strName, const std::string& sql_,
                     std::vector<std::vector<std::string> >* ret_data_,
                     std::string* errinfo, int* affectedRows_, std::vector<std::string>* col_)
{
    LOGTRACE((DB_MGR_LOG, "DbMgr::queryByName name<%s>, while sql<%s>", strName, sql_));
    AUTO_PERF();
    DBConnectionInfo* varDbConnection = NULL;
    {
        LockGuard lock(m_mutex);
        varDbConnection = getConnectionByName(strName);
    }

    if (NULL == varDbConnection)
    {
        return -1;
    }
    QueryDBResult result;
    varDbConnection->result_flag = 0;//!标记开始同步查询
    varDbConnection->tq->post(funcbind(&DbMgr::syncQueryDBImpl, this, varDbConnection, sql_, &result));
    varDbConnection->wait();
    if (ret_data_){
        *ret_data_ = result.dataResult;
    }
    if (errinfo){
        *errinfo = result.errinfo;
    }
    if (affectedRows_){
        *affectedRows_ = result.affectedRows;
    }
    if (col_){
        *col_ = result.fieldNames;
    }
    return 0;
}
void DbMgr::syncQueryDBImpl(DBConnectionInfo* varDbConnection, const string& sql_,  QueryDBResult* result)
{
    LOGTRACE((DB_MGR_LOG, "DbMgr::syncQueryDBImpl sql=%s", sql_));
    if (0 == varDbConnection->db->exeSql(sql_, result->dataResult, result->fieldNames))
    {
        result->affectedRows = varDbConnection->db->affectedRows();
    }
    else
    {
        result->errinfo = varDbConnection->db->errorMsg();
        LOGERROR((DB_MGR_LOG, "DbMgr::syncQueryDB failed<%s>, while sql<%s>", varDbConnection->db->errorMsg(), sql_));
    }
    varDbConnection->signal();
}
void DbMgr::DBConnectionInfo::signal()
{
    LockGuard lock(mutex);
    result_flag = 1;
    cond.signal();
}

int DbMgr::DBConnectionInfo::wait()
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

int DbMgr::getNumLike(const std::string& strName){
    int ret = 0;
    string nameLike = strName + "#";
    std::map<std::string, DBConnectionInfo*>::iterator it =  m_name2connection.begin();
    for (; it != m_name2connection.end(); ++it){
        if (it->first.find(nameLike) != string::npos){
            ++ ret;
        }
    }
    return ret;
}

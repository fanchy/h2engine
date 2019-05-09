
#ifndef _FF_SQLITE_OPS_H_
#define _FF_SQLITE_OPS_H_

#include "db/db_ops.h"
#include "db/sqlite3.h"

#include <vector>


namespace ff
{

class SqliteDB : public DbOpsI
{
public:
    struct callback_info_t
    {
        callback_info_t(SqliteDB* p = NULL):
            obj(p),
            callback(NULL)
        {}
        SqliteDB* obj;
        DbRowCallbackI* callback;
        std::vector<long>            length_buff;
    };
public:
    SqliteDB();
    virtual ~SqliteDB();

    virtual int  connect(const std::string& args_);
    virtual bool isConnected();
    virtual int  exeSql(const std::string& sql_, DbRowCallbackI* cb_);
    virtual void close();
    virtual int  affectedRows() { return m_affectedRows_num; }
    virtual const char*  errorMsg();

    void inc_affect_row_num()  { ++ m_affectedRows_num; }

    
    virtual void begin_transaction();
    virtual void commit_transaction();
    virtual void rollback_transaction();
    virtual int  ping() { return 0; }
private:
    void clear_env();
private:
    sqlite3* m_sqlite;
    bool     m_connected;
    std::string   m_error;
    int      m_affectedRows_num;
    callback_info_t m_callback_info;
};
}

#endif

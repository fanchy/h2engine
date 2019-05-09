#ifndef _MYSQL_OPS_H_
#define _MYSQL_OPS_H_

extern "C"{
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
}

#include <string>


#include "db/db_ops.h"
#include "base/lock.h"

namespace ff
{

class MysqlDB : public DbOpsI
{
    static Mutex g_mutex;

public:
    MysqlDB();
    virtual ~MysqlDB();

    virtual int  connect(const std::string& args_);
    virtual bool isConnected();
    virtual int  exeSql(const std::string& sql_, DbRowCallbackI* cb_);
    virtual void close();
    virtual int  affectedRows() { return m_affectedRows_num; }
    virtual const char*  errorMsg();
    
    virtual void begin_transaction();
    virtual void commit_transaction();
    virtual void rollback_transaction();
    virtual int  ping();
    
private:
    void clear_env();
private:
    std::string   m_host_arg;
    MYSQL    m_mysql;
    bool     m_connected;
    std::string   m_error;
    int      m_affectedRows_num;

};
}
#endif


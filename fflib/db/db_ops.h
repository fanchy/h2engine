#ifndef _FF_DB_OPS_H_
#define _FF_DB_OPS_H_

#include <string>


namespace ff
{

class DbRowCallbackI
{
public:
    virtual ~DbRowCallbackI() {}
    virtual void callback(int col_num_, char** col_datas_, char** col_names_, long* col_length_) = 0;
};

class DbOpsI
{
public:
    virtual ~DbOpsI(){}

    virtual int  connect(const std::string& args_) = 0;
    virtual bool isConnected() = 0;
    virtual int  exeSql(const std::string& sql_, DbRowCallbackI* cb_) = 0;
    virtual void close() = 0;
    virtual int  affectedRows() = 0;
    virtual const char*  errorMsg() = 0;

    virtual int  ping()                 = 0;
    virtual void begin_transaction()    = 0;
    virtual void commit_transaction()   = 0;
    virtual void rollback_transaction() = 0;
};

}
#endif




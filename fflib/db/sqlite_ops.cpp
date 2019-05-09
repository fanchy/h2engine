

#include "db/sqlite_ops.h"
#include <string.h>
using namespace std;
using namespace ff;

SqliteDB::SqliteDB():
    m_sqlite(NULL),
    m_connected(false),
    m_affectedRows_num(0)
{
    m_callback_info.obj = this;
}
SqliteDB::~SqliteDB()
{
    close();
}
void SqliteDB::clear_env()
{
    m_affectedRows_num = 0; 
    m_error.clear();
}

int SqliteDB::connect(const string& args_)
{
    clear_env();
    close();
    if (SQLITE_OK != ::sqlite3_open(args_.c_str(), &m_sqlite))
    {
        m_connected = false;
        m_error = sqlite3_errmsg(m_sqlite);
        return -1;
    }
    m_connected = true;
    return 0;
}

bool SqliteDB::isConnected()
{
    return m_connected;
}

static int default_callback(void* p_, int col_num_, char** col_datas_, char** col_names_)
{
    SqliteDB::callback_info_t* info = (SqliteDB::callback_info_t*)(p_);
    info->obj->inc_affect_row_num();
    if (info->callback)
    {
        for (int i = 0; i < col_num_; ++i)
        {
        	if (col_datas_[i])
        		info->length_buff.push_back(::strlen(col_datas_[i]));
        	else
        		info->length_buff.push_back(0);
        }
        long* ptr_length = col_num_ == 0? NULL: &(info->length_buff[0]);
        info->callback->callback(col_num_, col_datas_, col_names_, ptr_length);
        info->length_buff.clear();
    }
    return 0;
}

int  SqliteDB::exeSql(const string& sql_, DbRowCallbackI* cb_)
{
    char* msg = NULL;
    clear_env();
    m_callback_info.callback = cb_;
    if (::sqlite3_exec(m_sqlite, sql_.c_str(), &default_callback, &m_callback_info, &msg))
    {
        if (msg)
        {
            m_error = msg;
            ::sqlite3_free(msg);
        }
        return -1;
    }
    return 0;
}
void SqliteDB::close()
{
    if (m_sqlite)
    {
        ::sqlite3_close(m_sqlite);
        m_sqlite = NULL;
    }
}
const char*  SqliteDB::errorMsg()
{
    return m_error.c_str();
}


void SqliteDB::begin_transaction()
{
    exeSql("begin transaction", NULL);
}

void SqliteDB::commit_transaction()
{
    exeSql("commit transaction", NULL);
}

void SqliteDB::rollback_transaction()
{
    exeSql("rollback transaction", NULL);
}


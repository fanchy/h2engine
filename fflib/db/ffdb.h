
//! 封装db的操作
#ifndef _FFDB_H_
#define _FFDB_H_

#include <string>
#include <vector>


#include "db/db_ops.h"

namespace ff
{

class RowCommonCB: public DbRowCallbackI
{
public:
    RowCommonCB(std::vector<std::vector<std::string> >* data_, std::vector<std::string>* col_name_):
        m_all_data(data_),
        m_col_name(col_name_)
    {}
    virtual void callback(int col_num_, char** col_datas_, char** col_names_, long* col_length_)
    {
        (*m_all_data).push_back(std::vector<std::string>());
        std::vector<std::string>& data = (*m_all_data)[(*m_all_data).size() - 1];
        data.resize(col_num_);
        for (int i = 0; i < col_num_; ++i)
        {
            if (col_datas_[i])
            {
                data[i].assign(col_datas_[i], col_length_[i]);
            }
        }
        if (m_col_name && (*m_col_name).empty())
        {
            (*m_col_name).resize(col_num_);
            for (int i = 0; i < col_num_; ++i)
            {
                if (col_names_[i])
                    (*m_col_name)[i].assign(col_names_[i]);
            }
        }
    }
private:
    std::vector<std::vector<std::string> >* m_all_data;
    std::vector<std::string>*          m_col_name;
};

class FFDb
{
public:
    FFDb();
    ~FFDb();
    
    static std::string escape(const std::string& src_);
    static void dump(std::vector<std::vector<std::string> >& ret_data, std::vector<std::string>& col_names_);

    int  connect(const std::string& args_);
    bool isConnected();
    void close();
    int  affectedRows();
    const char*  errorMsg();

    int  exeSql(const std::string& sql_, DbRowCallbackI* cb_ = NULL);
    int  exeSql(const std::string& sql_, std::vector<std::vector<std::string> >& ret_data_);
    int  exeSql(const std::string& sql_, std::vector<std::vector<std::string> >& ret_data_, std::vector<std::string>& col_names_);
    
    int  ping();
private:
    DbOpsI*           m_db_ops;
};

}
#endif


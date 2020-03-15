
using System;
using System.Collections.Generic;

namespace ff
{
    public delegate void QueryCallback(List<string[]> rows, string[] colnames, string errorMsg);
    interface DbOps
    {
        bool Connect(string args_);
        bool IsConnected();
        bool  ExeSql(string sql_, QueryCallback cb);
        void Close();
        int  AffectedRows();
        string ErrorMsg();
    }

}

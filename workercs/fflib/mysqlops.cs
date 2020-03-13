
using System;
using System.Collections.Generic;
using MySql.Data.MySqlClient;

namespace ff
{
    class MysqlOps: DbOps
    {
        private MySqlConnection m_mysql;
        string m_strErr;
        int m_nLastAffectRowNum;
        public MysqlOps()
        {
            m_nLastAffectRowNum = 0;
        }
        public bool Connect(string args_)
        {
            if (m_mysql != null){
                m_mysql.Close();
                m_mysql = null;
            }
            bool ret = true;
            try
            {
                m_mysql = new MySqlConnection(args_);//("Database=guild;Data Source=109.244.2.71;port=5908;User Id=preader;Password=kFGZ6_57RCsrGJCU;CharSet=Latin1;");
                m_mysql.Open();
             }
            catch (System.Exception ex)
            {
                m_strErr = ex.Message;
                FFLog.Error("MysqlOps.Connect:" + ex.Message);
                ret = false;
                
                if (m_mysql != null){
                    m_mysql.Close();
                    m_mysql = null;
                }
            }
            return ret;
        }
        public bool IsConnected()
        {
            if (m_mysql != null && m_mysql.State == System.Data.ConnectionState.Open)
                return true;
            return false;
        }
        public bool  ExeSql(string sql_, QueryCallback cb)
        {
            m_strErr = "";
            m_nLastAffectRowNum = 0;
            try
            {
                MySqlCommand cmd = new MySqlCommand(sql_, m_mysql);
                if (cb == null)
                {
                    m_nLastAffectRowNum = cmd.ExecuteNonQuery();
                    return true;
                }
                MySqlDataReader reader =cmd.ExecuteReader();
                
                //int result =cmd.ExecuteNonQuery();//3.执行插入、删除、更改语句。执行成功返回受影响的数据的行数，返回1可做true判断。执行失败不返回任何数据，报错，下面代码都不执行
            
                m_nLastAffectRowNum = reader.RecordsAffected;
                int filedCount = reader.FieldCount;
                string[] colnames = new string[filedCount];
                for (int i = 0; i < filedCount; ++i)
                {
                    colnames[i] = reader.GetName(i);
                }
                List<string[]> result = new List<string[]>();
                while (reader.Read())//初始索引是-1，执行读取下一行数据，返回值是bool
                {
                    string[] row = new string[filedCount];
                    for (int i = 0; i < filedCount; ++i)
                    {
                        row[i] = reader[i].ToString();
                    }
                    result.Add(row);
                }
                cb(result, colnames);
            }
            catch (System.Exception ex)
            {
                m_strErr = ex.Message;
                FFLog.Error("MysqlOps.Connect:" + ex.Message);
                return false;
            }
            return true;
        }
        public void Close()
        {
            if (m_mysql != null){
                m_mysql.Close();
                m_mysql = null;
            }
        }
        public int  AffectedRows()
        {
            return m_nLastAffectRowNum;
        }
        public string ErrorMsg()
        {
            return m_strErr;
        }
    }

}
/*
try
{
    var Conn = new MysqlOps();
    Conn.Connect("Database=guild;Data Source=109.244.2.71;port=5908;User Id=preader;Password=kFGZ6_57RCsrGJCU;CharSet=Latin1;");
    Conn.ExeSql("select * from pyg50801 limit 2", (List<string[]> data, string[] colnames)=>{
        Console.WriteLine(colnames.ToString() + "-------" + data[1].ToString());
    });
    
}
catch (System.Exception ex)
{
    FFLog.Trace("void RunAllTask exception:" + ex.Message);
}
*/
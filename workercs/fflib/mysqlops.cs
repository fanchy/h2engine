
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
        public bool Connect(string args)
        {
            if (m_mysql != null){
                m_mysql.Close();
                m_mysql = null;
            }
            bool ret = true;
            try
            {
                string[] strList = args.Split("/");// mysql://127.0.0.1:3306/user/passwd/db
                if (strList.Length < 5)
                {
                    return false;
                }
                string[] host_vt = strList[2].Split(":");
                string host = host_vt[0];
                string port = "3306";
                if (host_vt.Length >= 2)
                {
                    port = host_vt[1];
                }
                string user   = strList[3];
                string passwd = strList[4];
                string db = "";
                if (strList.Length >= 6)
                {
                    db = strList[5];
                }
                string charset = "utf8";
                if (strList.Length >= 7)
                {
                    charset = strList[6];
                }
                string connectionString = string.Format("Database={0};Data Source={1};port={2};User Id={3};Password={4};CharSet={5};", db, host, port, user, passwd, charset);
                m_mysql = new MySqlConnection(connectionString);
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
    
    Conn.Connect("mysql://127.0.0.1:3306/user/password/dbname/charset");
    Conn.ExeSql("select * from test limit 2", (List<string[]> data, string[] colnames)=>{
        Console.WriteLine(colnames.ToString() + "-------" + data[1].ToString());
    });
    
}
catch (System.Exception ex)
{
    FFLog.Trace("void RunAllTask exception:" + ex.Message);
}
*/
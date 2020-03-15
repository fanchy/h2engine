using System;
using System.Collections.Generic;

namespace ff
{
    public class DBConnectionInfo
    {
        public TaskQueue            tq;
        public MysqlOps             db;
    };
    public class DBMgr
    {
        private static DBMgr gInstance = null;
        public static DBMgr Instance() {
            if (gInstance == null){
                gInstance = new DBMgr();
            }
            return gInstance;
        }
        private MysqlOps m_dbForSync;
        private DBConnectionInfo[] m_dbPool;
        public DBMgr()
        {

        }
        public bool Init()
        {
            return true;
        }
        public bool InitPool(string host, int nThreadNum)
        {
            m_dbForSync = new MysqlOps();
            if (!m_dbForSync.Connect(host))
            {
                FFLog.Error(string.Format("DbMgr::connectDB failed<%s>", m_dbForSync.ErrorMsg()));
                m_dbForSync = null;
                return false;
            }
            m_dbPool = new DBConnectionInfo[nThreadNum];
            for (int i = 0; i < nThreadNum; ++i)
            {
                MysqlOps db = new MysqlOps();
                if (!db.Connect(host))
                {
                    FFLog.Error(string.Format("DbMgr::connectDB failed<%s>", db.ErrorMsg()));
                    return false;
                }
                
                m_dbPool[i] = new DBConnectionInfo(){
                    tq = new TaskQueue(),
                    db = db
                };
                m_dbPool[i].tq.Run();
            }
            FFLog.Info(string.Format("DbMgr::connectDB host<%s>,num<%d>", host, nThreadNum));
            return true;
        }
        public bool AsyncQuery(Int64 modid, string sql, QueryCallback cb = null)
        {
            if (m_dbPool.Length == 0){
                return false;
            }
            DBConnectionInfo dbinfo = m_dbPool[modid % m_dbPool.Length];
            dbinfo.tq.Post(()=>{
                dbinfo.db.ExeSql(sql, cb);
            });
            return true;
        }
        public bool Query(string sql, QueryCallback cb = null)
        {
            return m_dbForSync.ExeSql(sql, cb);
        }
        public bool cleanup()
        {
            FFLog.Info("DbMgr::stop begin...");
            for (int i = 0; i < m_dbPool.Length; ++i)
            {
                m_dbPool[i].tq.Stop();
                m_dbPool[i].db.Close();
            }
            FFLog.Info("DbMgr::stop end");
            return true;
        }
    }
}
/*
DBMgr.Instance().InitPool("mysql://127.0.0.1:3306/user/password/dbname", 10);
DBMgr.Instance().AsyncQuery(1, "select * from table limit 10", (List<string[]> data, string[] colnames, string errMsg)=>{
    return;
});
*/

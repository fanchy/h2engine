using System;
using System.Net.Sockets;
using System.IO;
using System.Text;
namespace ff
{
    enum FFLogLevel
    {
        ERROR = 1,
        WARNING,
        INFO,
        TRACE,
        DEBUG,
    };
    class FFLog
    {
        private TaskQueue m_taskQueue;
        private FileStream m_fs;
        private StreamWriter m_sw;
        private int m_nLogLevel;
        public static FFLog gInstance = null;
        public static FFLog GetInstance()
        {
            if (gInstance == null)
            {
                gInstance = new FFLog();
            }
            return gInstance;
        }
        public FFLog(){
            m_fs = new FileStream("./test.log", FileMode.Append);
            m_sw = new StreamWriter(m_fs);

            m_taskQueue = new TaskQueue();
            m_taskQueue.Run();
            m_nLogLevel = (int)FFLogLevel.DEBUG;
        }
        ~FFLog()
        {
            DoCleanup();
        }
        void SetLogLevel(int n) { m_nLogLevel = n; }
        int  GetLogLevel() { return m_nLogLevel; }
        void DoCleanup()
        {
            if (m_taskQueue.IsRunning())
            {
                m_taskQueue.Stop();
                m_sw.Close();
                m_fs.Close();
            }
        }
        public void SetPath(string strPath)
        {
            m_fs.Close();
            m_sw.Close();
            m_fs = new FileStream(strPath, FileMode.Append);
            m_sw = new StreamWriter(m_fs);
        }
        public void LogToFile(FFLogLevel nLogLevel, string data)
        {
            if (m_nLogLevel < (int)nLogLevel)
            {
                return;
            }
            ConsoleColor color = ConsoleColor.Gray;
            string logdata = "";
            switch(nLogLevel)
            {
                case FFLogLevel.DEBUG:
                    {
                        logdata = string.Format("[{0:MM-dd HH:mm:ss}] DEBUG   {1}", System.DateTime.Now, data);
                    }break;
                case FFLogLevel.TRACE:
                    {
                        logdata = string.Format("[{0:MM-dd HH:mm:ss}] TRACE   {1}", System.DateTime.Now, data);
                    }break;
                case FFLogLevel.INFO:
                    {
                        color = ConsoleColor.Green;
                        logdata = string.Format("[{0:MM-dd HH:mm:ss}] INFO    {1}", System.DateTime.Now, data);
                    }
                    break;
                case FFLogLevel.WARNING:
                    {
                        color = ConsoleColor.Yellow;
                        logdata = string.Format("[{0:MM-dd HH:mm:ss}] WARNING {1}", System.DateTime.Now, data);
                    }
                    break;
                case FFLogLevel.ERROR:
                    {
                        color = ConsoleColor.Red;
                        logdata = string.Format("[{0:MM-dd HH:mm:ss}] ERROR   {1}", System.DateTime.Now, data);
                    }
                    break;
                default:
                    logdata = string.Format("[{0:MM-dd HH:mm:ss}] ERROR   {1}", System.DateTime.Now, data);
                    break;
            }
            
            m_taskQueue.Post(() =>{
                m_sw.WriteLine(logdata);
                m_sw.Flush();
                Console.ForegroundColor = color;
                Console.WriteLine(logdata);
            });
        }
        public static void Debug(string data)
        {
            FFLog.GetInstance().LogToFile(FFLogLevel.DEBUG, data);
        }
        public static void Trace(string data) {
            FFLog.GetInstance().LogToFile(FFLogLevel.TRACE, data);
        }
        public static void Info(string data)
        {
            FFLog.GetInstance().LogToFile(FFLogLevel.INFO, data);
        }
        public static void Warning(string data)
        {
            FFLog.GetInstance().LogToFile(FFLogLevel.WARNING, data);
        }
        public static void Error(string data)
        {
            FFLog.GetInstance().LogToFile(FFLogLevel.ERROR, data);
        }
        public static void Cleanup()
        {
            FFLog.GetInstance().DoCleanup();
        }
    }
}

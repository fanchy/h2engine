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
        public string m_strCurFileName;
        public static FFLog gInstance = null;
        public static FFLog Instance()
        {
            if (gInstance == null)
            {
                gInstance = new FFLog();
            }
            return gInstance;
        }
        public FFLog(){
            m_taskQueue = new TaskQueue();
            m_taskQueue.Run();
            m_nLogLevel = (int)FFLogLevel.DEBUG;
            m_strCurFileName = "";
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
        public void LogToFile(FFLogLevel nLogLevel, string data)
        {
            try
            {
                string fileName = string.Format("./log/{0:yyyy-MM-dd}.txt", System.DateTime.Now);
                if (m_fs == null || fileName != m_strCurFileName)
                {
                    if (!System.IO.Directory.Exists("./log")){
                        System.IO.Directory.CreateDirectory("./log");
                    }
                    m_strCurFileName = fileName;
                    if (m_fs != null){
                        m_fs.Close();
                    }
                    if (m_sw != null){
                        m_sw.Close();
                    }
                    m_fs = new FileStream(m_strCurFileName, FileMode.Append);
                    m_sw = new StreamWriter(m_fs);
                }
                
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
                            logdata = string.Format("[{0:yyyy-MM-dd HH:mm:ss}] DEBUG {1}", System.DateTime.Now, data);
                        }break;
                    case FFLogLevel.TRACE:
                        {
                            logdata = string.Format("[{0:yyyy-MM-dd HH:mm:ss}] TRACE {1}", System.DateTime.Now, data);
                        }break;
                    case FFLogLevel.INFO:
                        {
                            color = ConsoleColor.Green;
                            logdata = string.Format("[{0:yyyy-MM-dd HH:mm:ss}] INFO  {1}", System.DateTime.Now, data);
                        }
                        break;
                    case FFLogLevel.WARNING:
                        {
                            color = ConsoleColor.Yellow;
                            logdata = string.Format("[{0:yyyy-MM-dd HH:mm:ss}] WARN  {1}", System.DateTime.Now, data);
                        }
                        break;
                    case FFLogLevel.ERROR:
                        {
                            color = ConsoleColor.Red;
                            logdata = string.Format("[{0:yyyy-MM-dd HH:mm:ss}] ERROR {1}", System.DateTime.Now, data);
                        }
                        break;
                    default:
                        logdata = string.Format("[{0:yyyy-MM-dd HH:mm:ss}] ERROR {1}", System.DateTime.Now, data);
                        break;
                }
                
                m_taskQueue.Post(() =>{
                    m_sw.WriteLine(logdata);
                    m_sw.Flush();
                    Console.ForegroundColor = color;
                    Console.WriteLine(logdata);
                });
                }
            catch (System.Exception ex)
            {
                Console.WriteLine("Log.LogToFile Exception:" + ex.Message);
            }
        }
        public static void Debug(string data)
        {
            FFLog.Instance().LogToFile(FFLogLevel.DEBUG, data);
        }
        public static void Trace(string data) {
            FFLog.Instance().LogToFile(FFLogLevel.TRACE, data);
        }
        public static void Info(string data)
        {
            FFLog.Instance().LogToFile(FFLogLevel.INFO, data);
        }
        public static void Warning(string data)
        {
            FFLog.Instance().LogToFile(FFLogLevel.WARNING, data);
        }
        public static void Error(string data)
        {
            FFLog.Instance().LogToFile(FFLogLevel.ERROR, data);
        }
        public static void Cleanup()
        {
            FFLog.Instance().DoCleanup();
        }
    }
}

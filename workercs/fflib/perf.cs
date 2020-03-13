using System;
using System.IO;
using System.Collections.Generic;

namespace ff
{

    class PerfData
    {
        public PerfData(){
            max = 0;
            min = 2147483647;
            total = 0;
            times = 0;
        }
        public Int64 max;
        public Int64 min;
        public Int64 total;
        public Int64 times;
    }
    class PerfMonitor
    {
        protected Dictionary<string, PerfData> m_name2data;
        private bool bInit;
        PerfMonitor()
        {
            m_name2data = new Dictionary<string, PerfData>();
            bInit = false;
        }
        private static PerfMonitor gInstance = null;
        public static PerfMonitor Instance() {
            if (gInstance == null){
                gInstance = new PerfMonitor();
            }
            return gInstance;
        }
        public bool Init()
        {
            if (bInit)
                return true;
            bInit = true;
            
            FFNet.TimeroutLoop(10*60*1000, this.SaveLog, "PerfMonitor");
            return true;
        }
        public void AddPerf(string nameMod, Int64 us)
        {
            FFNet.GetTaskQueue().PostOrRunIfInSameThread(()=>{
                if (!bInit)
                    Init();
                if (us < 1)
                    us = 1;
                PerfData data = null;
                if (m_name2data.ContainsKey(nameMod))
                {
                    data = m_name2data[nameMod];
                }
                else{
                    data = new PerfData(){};
                    m_name2data[nameMod] = data;
                }
                data.total += us;
                data.times += 1;
                if (us > data.max)
                {
                    data.max = us;
                }
                if (us < data.min)
                {
                    data.min = us;
                }
            });
        }
        public void SaveLog()
        {
            if (m_name2data.Count == 0)
                return;
            
            try
            {
                string fileName = string.Format("./perf_{0:yyyy-MM-dd}.csv", System.DateTime.Now);
                bool bExistFile = System.IO.File.Exists(fileName);
                FileStream fs = new FileStream(fileName, FileMode.Append);
                StreamWriter sw = new StreamWriter(fs);

                if (!bExistFile)
                {
                    sw.WriteLine("time, mod, max, min, per, rps, times");
                }
                
                string timefmt = string.Format("{0:yyyy-MM-dd HH:mm:ss}", System.DateTime.Now);
                foreach(var kvp  in m_name2data)
                {
                    string nameMod = kvp.Key;
                    PerfData data = kvp.Value;
                    //! -------------------------- time, mod, max, min, per, rps, times
                    Int64 per = data.total / data.times;
                    Int64 rps = 0;
                    if (per > 0)
                        rps = 1000*1000 / per;
                    string logdata = string.Format("{0},{1},{2},{3},{4},{5},{6}", timefmt, nameMod, data.max, data.min, per, rps, data.times);
                    sw.WriteLine(logdata);
                }
                sw.Flush();
                sw.Close();
                fs.Close();
            }
            catch (System.Exception ex)
            {
                FFLog.Error("PerfMonitor.SaveLog:" + ex.Message);
            }
            m_name2data.Clear();
        }

    }

}

using System;
using System.Threading;
using System.Collections.Generic;

namespace ff
{
    public delegate void FFTask();
    class TaskQueue
    {
        private Mutex           m_mutex;
        private AutoResetEvent  m_event;
        private Thread          m_thread;
        private List<FFTask>    m_taskList;
        private volatile bool   m_running;
        public TaskQueue(){
            m_mutex = new Mutex();
            m_event = new AutoResetEvent(false);
            m_taskList = new List<FFTask>();
            m_running = false;
        }
        public bool IsRunning() { return m_running;  }
        public void Stop()
        {
            if (!m_running)
            {
                return;
            }

            m_running = false;
            m_event.Set();
            m_thread.Join();
        }
        public void Post(FFTask task)
        {
            int nsize = 0;
            m_mutex.WaitOne();
            nsize = m_taskList.Count;
            m_taskList.Add(task);
            if (nsize == 0)
            {
                m_event.Set();
            }

            m_mutex.ReleaseMutex();
        }
        public void Run()
        {
            if (m_running)
            {
                return;
            }

            m_running = true;
            m_thread = new Thread(new ThreadStart(RunAllTask));
            m_thread.Start();
        }
        private void RunAllTask()
        {
            List<FFTask> taskToRun = new List<FFTask>();
            while (m_running)
            {
                m_mutex.WaitOne();
                while (m_running && m_taskList.Count == 0)
                {
                    m_mutex.ReleaseMutex();
                    m_event.WaitOne(100);
                    m_mutex.WaitOne();
                }
                foreach (FFTask task in m_taskList)
                {
                    taskToRun.Add(task);
                }
                m_taskList.Clear();
                m_mutex.ReleaseMutex();
                foreach (FFTask task in taskToRun)
                {
                    try
                    {
                        task();
                    }
                    catch (System.Exception ex)
                    {
                        FFLog.Trace("void RunAllTask exception:" + ex.Message);
                        continue;
                    }
                    finally
                    {
                    }
                }
                taskToRun.Clear();
            }
        }
    }
}

using System;
using System.Runtime.InteropServices;

namespace ff
{
    public class FFMain
    {
        static string[] listEnableClassNames = {"RoleMgr", "MonsterMgr", "PlayerHandler"};
        public static void Main(string[] args)
        {
            string host = Util.strBrokerListen;
            FFBroker ffbroker = new FFBroker();
            ffbroker.Open(host);

            int nWorkerIndex = 0;
            if (FFWorker.Instance().Open(host, nWorkerIndex, listEnableClassNames) == false){
                FFLog.Trace("ffrpc open failed!");
            }

            //Console.ReadKey();
            //ffrpc.GetTaskQueue().Post(() =>
            //{
            //    SessionEnterWorkerReq reqMsg = new SessionEnterWorkerReq() { From_gate = "gate#0" };
            //    WorkerCallMsgReq reqWorkerCall = new WorkerCallMsgReq();
            //    //ffrpc.Call(strServiceName, reqMsg);
            //    reqMsg.From_gate = "gate#1";
            //    ffrpc.Call(strServiceName, reqWorkerCall, (SessionEnterWorkerReq retMsg) =>
            //    {
            //        FFLog.Trace(string.Format("ffrpc SessionEnterWorkerReq return!!!FromGate={0}", retMsg.From_gate));
            //    });
            //});
            FFGate ffGate = new FFGate();
            if (ffGate.Open(host, Util.strGateListen, 0) == false)
            {
                FFLog.Trace("ffGate open failed!");
            }
            

            //FFNet.Timerout(1000, Theout);
            //FFNet.Timerout(2000, Theout);
            // FFNet.Timerout(1000000, () =>
            // {
                // FFLog.Debug("AAAAAAAAAAAAAAA1");
                // //ffbroker.Close();
            // });
            //FFLog.Trace(string.Format("main! {0}", System.Threading.Thread.CurrentThread.ManagedThreadId.ToString()));

            bool bExit = false;
            AppDomain.CurrentDomain.ProcessExit += (sender, arg) =>
            {
                FFLog.Trace("exist!!");
                bExit = true;
            };
            Console.CancelKeyPress += (object sender, ConsoleCancelEventArgs e) =>{
                e.Cancel = true;
                FFLog.Trace("exist3!!");

                FFNet.Cleanup();
                FFLog.Cleanup();
                bExit = true;
            };
            while (!bExit)
            {
                System.Threading.Thread.Sleep(300);
            }

            FFLog.Trace("exist!!");
            FFNet.Cleanup();
            FFLog.Cleanup();
        }
        // static public void Theout()
        // {
            // FFLog.Trace(string.Format("theout! {0}", System.Threading.Thread.CurrentThread.ManagedThreadId.ToString()));
            // FFLog.Debug("AAAAAAAAAAAAAAA1");
            // FFLog.Trace("AAAAAAAAAAAAAAA2");
            // FFLog.Info("AAAAAAAAAAAAAAA3");
            // FFLog.Warning("AAAAAAAAAAAAAAA4");
            // FFLog.Error("AAAAAAAAAAAAAAA5");
        // }
        //[Xunit.Fact]
        // public void TestMax()
        // {
           // // Xunit.Assert.Equal(2, Math.Max(3, 2));
        // }
#if linux
        [DllImport("libc", SetLastError = true)]
        static extern int fork();
        
        [DllImport("libc", SetLastError = true)]
        static extern int setsid();
        
        [DllImport("libc", SetLastError = true)]
        static extern int umask(int mask);
        
        [DllImport("libc", SetLastError = true)]
        static extern int open([MarshalAs(UnmanagedType.LPStr)]string pathname, int flags);
        
        [DllImport("libc", SetLastError = true)]
        static extern int close(int fd);
        
        [DllImport("libc", SetLastError = true)]
        static extern int dup(int fd);
        
        [DllImport("libc", SetLastError = true)]
        static extern int exit(int code);
        
        [DllImport("libc", SetLastError = true)]
        static extern int execvp([MarshalAs(UnmanagedType.LPStr)]string file, string[] argv);
#endif
    }
}

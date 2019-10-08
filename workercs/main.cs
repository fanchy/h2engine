using System;
using System.Runtime.InteropServices;

namespace ff
{
    public class FFMain
    {
        const string DaemonTag = "--daemon";
        public static void Main(string[] args)
        {
#if linux
            if (args.Length >= 1 && (args[0] == "/daemon" || args[0] == "--daemon"))
            {
                FFLog.Trace(string.Format("1111 1 {0}", args[0]));
                // if (arg != "-daemon")
                    // continue;
                FFLog.Trace(string.Format("app arg:-daemon"));
                int pid = fork();
                if (pid != 0) exit(0);
                setsid();
                pid = fork();
                if (pid != 0) exit(0);
                umask(022);
                
                // ????????????
                int max = open("/dev/null", 0);
                for (var i = 0; i <= max; i++) { close(i); }
                
                //!read write error
                int fd = open("/dev/null", 0);
                dup(fd);
                dup(fd);
                
                var executablePath = Environment.GetCommandLineArgs()[0];
                FFLog.Trace(string.Format("executablePath {0}", executablePath));
                string[] args2 = {"mono", executablePath};

                execvp("mono", args2);
                return;
            }
#endif
            
            string host = "tcp://127.0.0.1:43210";
            FFBroker ffbroker = new FFBroker();
            ffbroker.Open(host);

            int nWorkerIndex = 0;
            FFWorker worker = new FFWorker();
            if (worker.Open(host, nWorkerIndex) == false){
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
            //        FFLog.Trace(string.Format("ffrpc SessionEnterWorkerReq return£¡£¡£¡FromGate={0}", retMsg.From_gate));
            //    });
            //});
            FFGate ffGate = new FFGate();
            if (ffGate.Open(host, "tcp://*:44000", 0) == false)
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

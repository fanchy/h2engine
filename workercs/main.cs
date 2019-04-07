using System;



namespace ff
{
    public class FFMain
    {
        public static void Main(string[] args)
        {
            string host = "tcp://127.0.0.1:43210";
            FFBroker ffbroker = new FFBroker();
            ffbroker.Open(host);

            string strServiceName = "worker#0";
            FFRpc ffrpc = new FFRpc(strServiceName);
            if (ffrpc.Open(host) == false){
                FFLog.Trace("ffrpc open failed!");
            }
            ffrpc.Reg((SessionEnterWorkerReq req) =>
            {
                FFLog.Trace(string.Format("ffrpc SessionEnterWorkerReq £¡£¡£¡FromGate={0}", req.From_gate));
                return req;
            });
            Console.ReadKey();
            ffrpc.GetTaskQueue().Post(() =>
            {
                SessionEnterWorkerReq reqMsg = new SessionEnterWorkerReq() { From_gate = "gate#0" };
                WorkerCallMsgReq reqWorkerCall = new WorkerCallMsgReq();
                //ffrpc.Call(strServiceName, reqMsg);
                reqMsg.From_gate = "gate#1";
                ffrpc.Call(strServiceName, reqWorkerCall, (SessionEnterWorkerReq retMsg) =>
                {
                    FFLog.Trace(string.Format("ffrpc SessionEnterWorkerReq return£¡£¡£¡FromGate={0}", retMsg.From_gate));
                });
            });

            //FFNet.Timerout(1000, Theout);
            //FFNet.Timerout(2000, Theout);
            FFNet.Timerout(100000, () =>
            {
                FFLog.Debug("AAAAAAAAAAAAAAA1");
                ffrpc.Close();
            });
            FFLog.Trace(string.Format("main! {0}", System.Threading.Thread.CurrentThread.ManagedThreadId.ToString()));
            
            AppDomain.CurrentDomain.ProcessExit += (sender, arg) =>
            {
                FFLog.Trace("exist!!");
            };
            Console.CancelKeyPress += (object sender, ConsoleCancelEventArgs e) =>{
                e.Cancel = true;
                FFLog.Trace("exist3!!");

                FFNet.Cleanup();
                FFLog.Cleanup();
            };
            Console.ReadKey();
            FFLog.Trace("exist!!");
            FFNet.Cleanup();
            FFLog.Cleanup();
        }
        static public void Theout()
        {
            FFLog.Trace(string.Format("theout! {0}", System.Threading.Thread.CurrentThread.ManagedThreadId.ToString()));
            FFLog.Debug("AAAAAAAAAAAAAAA1");
            FFLog.Trace("AAAAAAAAAAAAAAA2");
            FFLog.Info("AAAAAAAAAAAAAAA3");
            FFLog.Warning("AAAAAAAAAAAAAAA4");
            FFLog.Error("AAAAAAAAAAAAAAA5");
        }
        //[Xunit.Fact]
        public void TestMax()
        {
           // Xunit.Assert.Equal(2, Math.Max(3, 2));
        }

    }
}

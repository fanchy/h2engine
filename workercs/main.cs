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
            FFRpc ffrpc = new FFRpc("worker#0");
            if (ffrpc.Open(host) == false){
                FFLog.Trace("ffrpc open failed!");
            }

            FFNet.Timerout(1000, Theout);
            FFNet.Timerout(2000, Theout);
            FFNet.Timerout(5000, Theout);
            FFLog.Trace(string.Format("main! {0}", System.Threading.Thread.CurrentThread.ManagedThreadId.ToString()));
            Console.ReadKey();
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
        public static void OnRecv(IFFSocket ffsocket, Int16 cmd, string strData){
            FFLog.Trace(string.Format("onRecv....{0}, {1}", strData, cmd));
            FFNet.SendMsg(ffsocket, cmd, strData);
        }

        public static void OnRecv2(IFFSocket ffsocket, Int16 cmd, string strData){
            FFLog.Trace(string.Format("onRecv2....{0}, {1}", strData, cmd));
        }
        public static void OnBroken(IFFSocket ffsocket){
            FFLog.Trace("onBroken....");
        }
        public static void OnBroken2(IFFSocket ffsocket){
            FFLog.Trace("onBroken2....");
        }
        //[Xunit.Fact]
        public void TestMax()
        {
           // Xunit.Assert.Equal(2, Math.Max(3, 2));
        }

    }
}

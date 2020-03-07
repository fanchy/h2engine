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

            FFGate ffGate = new FFGate();
            if (ffGate.Open(host, Util.strGateListen, 0) == false)
            {
                FFLog.Trace("ffGate open failed!");
            }
            
            bool bExit = false;
            AppDomain.CurrentDomain.ProcessExit += (sender, arg) =>
            {
                FFLog.Trace("exit!");
                bExit = true;
            };
            Console.CancelKeyPress += (object sender, ConsoleCancelEventArgs e) =>{
                e.Cancel = true;
                FFLog.Trace("exit!!");
                bExit = true;
            };
            while (!bExit)
            {
                System.Threading.Thread.Sleep(300);
            }

            FFLog.Trace("exit!!!");
            FFNet.Cleanup();
            FFLog.Cleanup();
        }
    }
}

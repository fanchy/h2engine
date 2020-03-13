using System;
using System.Collections.Generic;

namespace ff
{
    public class FFMain
    {
        static readonly string[] listEnableClassNames = {"RoleMgr", "MonsterMgr", "PlayerHandler"};
        public static void Main(string[] args)
        {
            string host = Util.strBrokerListen;
            if (!FFBroker.Instance().Init(host)){
                FFLog.Error("FFBroker open failed!");
                return;
            }

            int nWorkerIndex = 0;
            if (FFWorker.Instance().Init(host, nWorkerIndex, listEnableClassNames) == false){
                FFLog.Trace("FFWorker open failed!");
                return;
            }

            if (FFGate.Instance().Init(host, Util.strGateListen) == false)
            {
                FFLog.Trace("ffGate open failed!");
                return;
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
            FFGate.Instance().Cleanup();
            FFBroker.Instance().Cleanup();
            FFWorker.Instance().Cleanup();
            FFNet.Cleanup();
            FFLog.Cleanup();
        }
    }
}

using System;
using System.Collections.Generic;

namespace ff
{
    public class FFApp
    {
        static readonly string[] listEnableClassNames = {"DBMgr", "RoleMgr", "MonsterMgr", "PlayerHandler"};
        public static void Main(string[] args)
        {
            CfgTool.Instance().InitCfg(args);
            string strBrokerListen = CfgTool.Instance().GetCfgVal("BrokerListen", "tcp://127.0.0.1:4321");
            if (!FFBroker.Instance().Init(strBrokerListen)){
                FFLog.Error("FFBroker open failed!");
                return;
            }

            int nWorkerIndex = 0;
            if (FFWorker.Instance().Init(strBrokerListen, nWorkerIndex, listEnableClassNames) == false){
                FFLog.Trace("FFWorker open failed!");
                return;
            }

            string strGateListen = CfgTool.Instance().GetCfgVal("GateListen", "tcp://*:44000");
            if (FFGate.Instance().Init(strBrokerListen, strGateListen) == false)
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

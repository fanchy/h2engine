using System;
using System.Net.Sockets;

namespace ff
{

    enum FFRPC_DEF
    {
        BROKER_TO_CLIENT_MSG = 1,
        //!新版本*********
        REGISTER_TO_BROKER_REQ,
        REGISTER_TO_BROKER_RET,
        BROKER_ROUTE_MSG,
        SYNC_CLIENT_REQ, //! 同步客户端的请求，如python,php
    };
    class FFRpc
    {
        public FFSocket m_socketBroker;
        public string   m_strServiceName;
        public string   m_strBrokerIP;
        public int      m_nBrokerPort;
        public FFRpc(string strName){
            m_strServiceName = strName;
            m_strBrokerIP    = "127.0.0.1";
            m_nBrokerPort    = 43210;
        }
        public bool open(string strBrokerCfg){
            string[] strList = strBrokerCfg.Split(":");
            if (strList.Length != 2){
                return false;
            }
            m_strBrokerIP = strList[0];
            m_nBrokerPort = int.Parse(strList[1]);
            Console.WriteLine("open....{0}, {1}", m_strBrokerIP, m_nBrokerPort);
            if (!connectToBroker()){
                return false;
            }
            return true;
        }
        public bool connectToBroker(){
            m_socketBroker = FFNet.connect(m_strBrokerIP, m_nBrokerPort, new SocketMsgHandler(handleMsg), new SocketBrokenHandler(handleBroken));
            if (null == m_socketBroker){
                return false;
            }
            Console.WriteLine("connect....{0}, {1}", m_strBrokerIP, m_nBrokerPort);
            RegisterToBrokerReq reqMsg = new RegisterToBrokerReq();
            reqMsg.Node_type = 2;
            reqMsg.Service_name = m_strServiceName;

            FFNet.sendMsg(m_socketBroker, 2, reqMsg);
            Console.WriteLine("send....{0}, {1}", m_strBrokerIP, m_nBrokerPort);
            return true;
        }
        public bool close(){
            return true;
        }
        public void handleMsg(FFSocket ffsocket, Int16 cmd, string strMsg){
            Console.WriteLine("handleMsg....{0}, {1}", cmd, strMsg.Length);
        }
        public void handleBroken(FFSocket ffsocket){

        }
    }
}

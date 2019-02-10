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
        public long     m_nNodeID;
        public RegisterToBrokerRet m_brokerData;
        public FFRpc(string strName){
            m_strServiceName = strName;
            m_strBrokerIP    = "127.0.0.1";
            m_nBrokerPort    = 43210;
            m_brokerData = new RegisterToBrokerRet();
        }
        public bool open(string strBrokerCfg){
            string[] strList = strBrokerCfg.Split(":");
            if (strList.Length != 2){
                return false;
            }
            m_strBrokerIP = strList[0];
            m_nBrokerPort = int.Parse(strList[1]);
            Console.WriteLine("ffrpc open....{0}, {1}", m_strBrokerIP, m_nBrokerPort);
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
            RegisterToBrokerReq reqMsg = new RegisterToBrokerReq();
            reqMsg.Node_type = 2;
            reqMsg.Service_name = m_strServiceName;

            FFNet.sendMsg(m_socketBroker, 2, reqMsg);
            return true;
        }
        public bool close(){
            return true;
        }
        public void handleMsg(FFSocket ffsocket, Int16 cmd, string strMsg){
            Console.WriteLine("handleMsg....{0}, {1}", cmd, strMsg.Length);
            if (cmd == 3){
                FFNet.decodeMsg(m_brokerData, strMsg);
                Console.WriteLine("handleMsg....{0}, {1}", m_brokerData.Node_id, m_brokerData.Register_flag);
                if (m_brokerData.Register_flag == 1)
                {
                    m_nNodeID = m_brokerData.Node_id;//! -1表示注册失败，0表示同步消息，1表示注册成功
                }
            }
        }
        public void handleBroken(FFSocket ffsocket){

        }
    }
}

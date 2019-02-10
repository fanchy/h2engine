using System;
using System.Net.Sockets;

namespace ff
{
    class FFRpc
    {
        public FFSocket m_socketBroker;
        public string   m_strServiceName;
        public string   m_strBrokerIP;
        public int      m_nBrokerPort;
        public FFRpc(string strName){
            m_strServiceName = strName;
            m_strBrokerIP    = "";
            m_nBrokerPort    = 43210;
        }
        bool open(string strBrokerCfg){
            if (!connectToBroker()){
                return false;
            }
            return true;
        }
        bool connectToBroker(){
            m_socketBroker = FFNet.connect(m_strBrokerIP, m_nBrokerPort, new SocketMsgHandler(handleMsg), new SocketBrokenHandler(handleBroken));
            if (null == m_socketBroker){
                return false;
            }
            FFNet.sendMsg(m_socketBroker, 10, "");
            return true;
        }
        bool close(){
            return true;
        }
        void handleMsg(FFSocket ffsocket, Int16 cmd, string strMsg){

        }
        void handleBroken(FFSocket ffsocket){

        }
    }
}

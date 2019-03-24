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
        public IFFSocket m_socketBroker;
        public string   m_strServiceName;
        public string   m_strBrokerHost;
        public long     m_nNodeID;
        public RegisterToBrokerRet m_brokerData;
        public FFRpc(string strName){
            m_strServiceName = strName;
            m_strBrokerHost  = "tcp://127.0.0.1:43210";
            m_brokerData = new RegisterToBrokerRet();
        }
        public bool Open(string strBrokerCfg){
            if (strBrokerCfg.Length > 0)
            {
                m_strBrokerHost = strBrokerCfg;
            }

            FFLog.Trace(string.Format("ffrpc open....{0} begin", m_strBrokerHost));
            if (!ConnectToBroker()){
                return false;
            }
            return true;
        }
        public bool ConnectToBroker() {
            m_socketBroker = FFNet.Connect(m_strBrokerHost, new SocketMsgHandler(HandleMsg), new SocketBrokenHandler(HandleBroken));
            if (null == m_socketBroker) {
                return false;
            }
            RegisterToBrokerReq reqMsg = new RegisterToBrokerReq{ Node_type = 2, Service_name = m_strServiceName};
            //reqMsg.Node_type = 2;
            //reqMsg.Service_name = m_strServiceName;

            FFNet.SendMsg(m_socketBroker, 2, reqMsg);
            return true;
        }
        public bool Close(){
            return true;
        }
        public void HandleMsg(IFFSocket ffsocket, Int16 cmd, string strMsg){
            FFLog.Trace(string.Format("FFRpc handleMsg....{0}, {1} [{2}]", cmd, strMsg.Length, System.Threading.Thread.CurrentThread.ManagedThreadId.ToString()));
            if (cmd == (int)FFRPC_DEF.BROKER_ROUTE_MSG)
            {
                FFNet.DecodeMsg(m_brokerData, strMsg);
                FFLog.Trace(string.Format("handleMsg....{0}, {1}", m_brokerData.Node_id, m_brokerData.Register_flag));
                if (m_brokerData.Register_flag == 1)
                {
                    m_nNodeID = m_brokerData.Node_id;//! -1表示注册失败，0表示同步消息，1表示注册成功
                }
            }
        }
        public void HandleBroken(IFFSocket ffsocket){

        }
    }
}

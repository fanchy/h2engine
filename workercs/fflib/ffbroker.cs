using System;
using System.Net.Sockets;

namespace ff
{
    class FFBroker
    {
        public string   m_strListenHost;
        public RegisterToBrokerRet m_brokerData;
        public FFAcceptor m_acceptor;
        public FFBroker(){
            m_strListenHost = "tcp://127.0.0.1:43210";
            m_brokerData = new RegisterToBrokerRet();
        }
        public bool Open(string strBrokerCfg) {
            if (strBrokerCfg.Length > 0)
            {
                m_strListenHost = strBrokerCfg;
            }

            m_acceptor = FFNet.Listen(m_strListenHost, new SocketMsgHandler(HandleMsg), new SocketBrokenHandler(HandleBroken));
            if (m_acceptor != null)
            {
                FFLog.Trace(string.Format("FFBroker open....{0} ok", m_strListenHost));
            }
            else
            {
                FFLog.Trace(string.Format("FFBroker open....{0} failed", m_strListenHost));
            }
            return true;
        }
        public bool Close(){
            return true;
        }
        public void HandleMsg(IFFSocket ffsocket, Int16 cmd, string strMsg){
            FFLog.Trace(string.Format("FFBroker handleMsg....{0}, {1} [{2}]", cmd, strMsg.Length, System.Threading.Thread.CurrentThread.ManagedThreadId.ToString()));
            if (cmd == 3){
                FFNet.DecodeMsg(m_brokerData, strMsg);
                FFLog.Trace(string.Format("handleMsg....{0}, {1}", m_brokerData.Node_id, m_brokerData.Register_flag));
                if (m_brokerData.Register_flag == 1)
                {
                    //m_nNodeID = m_brokerData.Node_id;//! -1表示注册失败，0表示同步消息，1表示注册成功
                }
            }
        }
        public void HandleBroken(IFFSocket ffsocket){

        }
    }
}

using System;
using System.Net.Sockets;

namespace ff
{
    public delegate void SocketMsgHandler(FFSocket ffsocket, Int16 cmd, string strData);
    class SocketCtrl
    {
        public Int32 size;
        public Int16 cmd;
        public Int16 flag;
        public string m_strRecvData;
        public SocketMsgHandler         m_funcMsgHandler;
        public SocketBrokenHandler      m_funcBroken;
        public SocketCtrl(SocketMsgHandler funcMsg, SocketBrokenHandler funcBroken){
            size = 0;
            cmd  = 0;
            flag = 0;
            m_strRecvData    = "";
            m_funcMsgHandler = funcMsg;
            m_funcBroken     = funcBroken;
        }
        public void handleRecv(FFSocket ffsocket, string strData){
            m_strRecvData += strData;
            while (m_strRecvData.Length >= 8){
                byte[] btValue = System.Text.Encoding.UTF8.GetBytes(m_strRecvData);
                size = System.Net.IPAddress.NetworkToHostOrder(BitConverter.ToInt32(btValue, 0));
                cmd  = System.Net.IPAddress.NetworkToHostOrder(BitConverter.ToInt16(btValue, 4));
                flag = System.Net.IPAddress.NetworkToHostOrder(BitConverter.ToInt16(btValue, 6));
                if (m_strRecvData.Length < 8 + size){
                    break;
                }
                string strMsg = m_strRecvData.Remove(0, 8);
                if (strMsg.Length == size){
                    m_strRecvData = "";
                }
                else{
                    strMsg = strMsg.Remove(size);
                    m_strRecvData = m_strRecvData.Remove(0, 8 + size);
                }
                m_funcMsgHandler(ffsocket, cmd, strMsg);
            }
        }
        public void handleBroken(FFSocket ffsocket){
            m_funcBroken(ffsocket);
        }
    }

    class NetOps
    {
        public static FFSocket connect(string ip, int port, SocketMsgHandler funcMsg, SocketBrokenHandler funcBroken){
            SocketCtrl ctrl = new SocketCtrl(funcMsg, funcBroken);
            FFSocket ffsocket = new FFScoketAsync(new SocketRecvHandler(ctrl.handleRecv), new SocketBrokenHandler(ctrl.handleBroken));
            if (ffsocket.connect(ip, port)){
                return ffsocket;
            }
            return null;
        }
        public static FFAcceptor listen(string ip, int port, SocketMsgHandler funcMsg, SocketBrokenHandler funcBroken){
            SocketCtrl ctrl = new SocketCtrl(funcMsg, funcBroken);
            FFAcceptor ffacceptor = new FFAcceptor(new SocketRecvHandler(ctrl.handleRecv), new SocketBrokenHandler(ctrl.handleBroken));
            if (ffacceptor.listen(ip, port)){
                return ffacceptor;
            }
            return null;
        }
        public static void sendMsg(FFSocket ffsocket, Int16 cmd, string strData){
            int len = strData.Length;
            len = System.Net.IPAddress.HostToNetworkOrder(len);
            cmd = System.Net.IPAddress.HostToNetworkOrder(cmd);
            byte[] lenArray = BitConverter.GetBytes(len);
            byte[] cmdArray = BitConverter.GetBytes(cmd);
            byte[] resArray = new byte[2]{0, 0};
            string strRet = System.Text.Encoding.UTF8.GetString(lenArray, 0, lenArray.Length);
            //Console.WriteLine("sendMsg {0}, {1}", strRet.Length, strData.Length);
            strRet += System.Text.Encoding.UTF8.GetString(cmdArray, 0, cmdArray.Length);
            //Console.WriteLine("sendMsg {0}, {1}", strRet.Length, strData.Length);
            strRet += System.Text.Encoding.UTF8.GetString(resArray, 0, resArray.Length);
            //Console.WriteLine("sendMsg {0}, {1}", strRet.Length, strData.Length);
            strRet += strData;
            Console.WriteLine("sendMsg {0}, {1}", strRet.Length, strData.Length);

            ffsocket.asyncSend(strRet);
        }
    }
}

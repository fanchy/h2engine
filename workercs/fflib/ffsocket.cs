using System;
using System.Net.Sockets;

namespace ff
{
    public interface IFFSocket{
        bool Connect(string ip, int port);
        void AsyncRecv();
        void AsyncSend(string strData);
        void Close();
    }
    public delegate void SocketRecvHandler(IFFSocket ffsocket, string strData);
    public delegate void SocketBrokenHandler(IFFSocket ffsocket);
    class FFScoketAsync: IFFSocket
    {
        protected Socket                        m_oSocket;
        protected byte[]                        m_oBuffer;
        protected string                        m_oBuffSending;
        protected string                        m_oBuffWaiting;
        protected SocketRecvHandler             m_funcRecv;
        protected SocketBrokenHandler           m_funcBroken;
        public FFScoketAsync(SocketRecvHandler onRecv, SocketBrokenHandler onBroken, Socket socket = null){
            if (socket == null)
            {
                m_oSocket   = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            }
            else
            {
                m_oSocket   = socket;
            }

            m_oBuffer       = new byte[1024];
            m_oBuffSending  = "";
            m_oBuffWaiting  = "";
            m_funcRecv      = onRecv;
            m_funcBroken    = onBroken;
        }
        public bool Connect(string ip, int port){
            try{
                m_oSocket.Connect(ip, port);
            }
            catch (Exception ex)
            {
                FFLog.Trace("scoket: connect Error " + ex.Message);
                return false;
            }
            AsyncRecv();
            return true;
        }
        public void Close(){
            HandleClose();
        }
        public void AsyncRecv(){
            m_oSocket.BeginReceive(m_oBuffer, 0, m_oBuffer.Length, SocketFlags.None, new AsyncCallback(HandleRecv), m_oSocket);
        }
        public void PostMsg(string data){
            m_funcRecv(this, data);
        }
        public void HandleRecv(IAsyncResult ar)
        {
            var length = 0;
            try
            {
                Socket socket = (Socket)ar.AsyncState;
                if (socket == null)
                {

                    return;
                }
                length = socket.EndReceive(ar);
            }
            catch (Exception ex)
            {
                FFLog.Trace("scoket: recv Error " + ex.Message);
                HandleClose();
                return;
            }

            FFNet.GetTaskQueue().Post(() =>
            {
                try
                {
                    var message = System.Text.Encoding.UTF8.GetString(m_oBuffer, 0, length);
                    PostMsg(message);

                    //接收下一个消息
                    if (m_oSocket != null)
                    {
                        m_oSocket.BeginReceive(m_oBuffer, 0, m_oBuffer.Length, SocketFlags.None, new AsyncCallback(HandleRecv), m_oSocket);
                    }
                }
                catch (Exception ex)
                {
                    FFLog.Trace("scoket: recv Error " + ex.Message);
                    HandleClose();
                }
            });
        }
        public void AsyncSend(string strData){
            FFNet.GetTaskQueue().Post(() =>
            {
                if (strData.Length == 0 || m_oSocket == null)
                {
                    return;
                }

                if (m_oBuffSending.Length == 0)
                {
                    m_oBuffSending = strData;
                    byte[] data = System.Text.Encoding.UTF8.GetBytes(m_oBuffSending);
                    m_oSocket.BeginSend(data, 0, data.Length, 0, new AsyncCallback(handleSendEnd), m_oSocket);
                }
                else
                {
                    m_oBuffWaiting += strData;
                }
            });
        }
        private void handleSendEnd(IAsyncResult ar)
        {
            try
            {
                var socket = ar.AsyncState as Socket;
                socket.EndSend(ar);
            }
            catch (SocketException ex)
            {
                FFLog.Trace("scoket: send Error " + ex.Message);
                HandleClose();
                return;
            }
            FFNet.GetTaskQueue().Post(() =>
            {
                m_oBuffSending = m_oBuffWaiting;
                m_oBuffWaiting = "";
                try
                {
                    if (m_oBuffSending.Length > 0 && m_oSocket != null)
                    {
                        byte[] data = System.Text.Encoding.UTF8.GetBytes(m_oBuffSending);
                        m_oSocket.BeginSend(data, 0, data.Length, 0, new AsyncCallback(handleSendEnd), m_oSocket);
                    }
                }
                catch (SocketException ex)
                {
                    FFLog.Trace("scoket: send Error " + ex.Message);
                    HandleClose();
                }
            });
        }
        public void HandleClose(){
            FFNet.GetTaskQueue().Post(() =>
            {
                if (m_oSocket == null)
                {
                    return;
                }

                m_oSocket.Close();
                m_oSocket = null;
                m_oBuffSending = "";
                m_oBuffWaiting = "";
                m_funcBroken(this);
            });
        }

    }
}

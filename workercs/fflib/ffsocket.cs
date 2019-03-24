using System;
using System.Net.Sockets;

namespace ff
{
    public interface FFSocket{
        bool connect(string ip, int port);
        void asyncRecv();
        void asyncSend(string strData);
        void close();
    }
    public delegate void SocketRecvHandler(FFSocket ffsocket, string strData);
    public delegate void SocketBrokenHandler(FFSocket ffsocket);
    class FFScoketAsync: FFSocket
    {
        protected Socket                        m_oSocket;
        protected byte[]                        m_oBuffer;
        protected string                        m_oBuffSending;
        protected string                        m_oBuffWaiting;
        protected SocketRecvHandler             m_funcRecv;
        protected SocketBrokenHandler           m_funcBroken;
        public FFScoketAsync(SocketRecvHandler onRecv, SocketBrokenHandler onBroken, Socket socket = null){
            if (socket == null)
                m_oSocket   = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            else
                m_oSocket   = socket;
            m_oBuffer       = new byte[1024];
            m_oBuffSending  = "";
            m_oBuffWaiting  = "";
            m_funcRecv      = onRecv;
            m_funcBroken    = onBroken;
        }
        public bool connect(string ip, int port){
            try{
                m_oSocket.Connect(ip, port);
            }
            catch (Exception ex)
            {
                WriteLine("scoket: connect Error " + ex.Message, ConsoleColor.Red);
                return false;
            }
            asyncRecv();
            return true;
        }
        public void close(){
            try {
                m_oSocket.Close();
            }
            catch (Exception ex)
            {
                WriteLine("scoket: close Error " + ex.Message, ConsoleColor.Red);
            }
        }
        public void asyncRecv(){
            m_oSocket.BeginReceive(m_oBuffer, 0, m_oBuffer.Length, SocketFlags.None, new AsyncCallback(handleRecv), m_oSocket);
        }
        public void postMsg(string data){
            m_funcRecv(this, data);
        }
        public void handleRecv(IAsyncResult ar)
        {
            try
            {
                var socket = ar.AsyncState as Socket;

                //方法参考：http://msdn.microsoft.com/zh-cn/library/system.net.sockets.socket.endreceive.aspx
                if (socket != null)
                {
                    var length = socket.EndReceive(ar);
                    if (length <= 0){
                        handleClose();
                        return;
                    }
                    var message = System.Text.Encoding.UTF8.GetString(m_oBuffer, 0, length);
                    postMsg(message);
                }

                //接收下一个消息
                if (m_oSocket != null)
                {
                    m_oSocket.BeginReceive(m_oBuffer, 0, m_oBuffer.Length, SocketFlags.None, new AsyncCallback(handleRecv), m_oSocket);
                }
            }
            catch (Exception ex)
            {
                WriteLine("scoket: recv Error " + ex.Message, ConsoleColor.Red);
                handleClose();
            }
        }
        public void asyncSend(string strData){
            if (strData.Length == 0 || m_oSocket == null)
                return;
            if (m_oBuffSending.Length == 0){
                m_oBuffSending = strData;
                byte[] data = System.Text.Encoding.UTF8.GetBytes(m_oBuffSending);
                m_oSocket.BeginSend(data, 0, data.Length, 0, new AsyncCallback(handleSendEnd), m_oSocket);
            }
            else{
                m_oBuffWaiting += strData;
            }
        }
        private void handleSendEnd(IAsyncResult ar)
        {
            m_oBuffSending = m_oBuffWaiting;
            m_oBuffWaiting = "";
            try
            {
                var socket = ar.AsyncState as Socket;
                socket.EndSend(ar);
                if (m_oBuffSending.Length > 0 && m_oSocket != null){
                    byte[] data = System.Text.Encoding.UTF8.GetBytes(m_oBuffSending);
                    m_oSocket.BeginSend(data, 0, data.Length, 0, new AsyncCallback(handleSendEnd), m_oSocket);
                }
            }
            catch (SocketException ex)
            {
                WriteLine("scoket: send Error " + ex.Message, ConsoleColor.Red);
                handleClose();
            }
        }
        public void handleClose(){
            if (m_oSocket == null)
                return;
            m_oSocket = null;
            m_oBuffSending = "";
            m_oBuffWaiting = "";
            m_funcBroken(this);
        }

        public static void WriteLine(string str, ConsoleColor color = ConsoleColor.Black)
        {
            Console.ForegroundColor = color;
            Console.WriteLine("[{0:MM-dd HH:mm:ss}] {1}", DateTime.Now, str);
        }
    }
}

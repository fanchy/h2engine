using System;
using System.Net.Sockets;

namespace ff
{
    class FFAcceptor
    {
        protected Socket              m_oSocket;
        protected SocketRecvHandler   m_funcRecv;
        protected SocketBrokenHandler m_funcBroken;
        public FFAcceptor(SocketRecvHandler onRecv, SocketBrokenHandler onBroken){
            m_oSocket   = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            m_funcRecv  = onRecv;
            m_funcBroken= onBroken;
        }
        public bool listen(string ip, int port){
            try{
                if (ip == "*" || ip == ""){
                    m_oSocket.Bind(new System.Net.IPEndPoint(System.Net.IPAddress.Any, port));
                }
                else{
                    m_oSocket.Bind(new System.Net.IPEndPoint(System.Net.IPAddress.Parse(ip), port));
                }
                m_oSocket.Listen(2);
                m_oSocket.BeginAccept(new AsyncCallback(handleAccepted), m_oSocket);
            }
            catch (Exception ex)
            {
                WriteLine("scoket: listen Error " + ex.Message, ConsoleColor.Red);
                return false;
            }
            return true;
        }
        public void handleAccepted(IAsyncResult ar)
        {
            try
            {
                var socket = ar.AsyncState as Socket;

                //方法参考：http://msdn.microsoft.com/zh-cn/library/system.net.sockets.socket.endreceive.aspx
                if (socket != null)
                {
                    var client = socket.EndAccept(ar);
                    FFSocket ffsocket = new FFScoketAsync(m_funcRecv, m_funcBroken, client);
                    ffsocket.asyncRecv();
                    m_oSocket.BeginAccept(new AsyncCallback(handleAccepted), m_oSocket);
                }
            }
            catch (Exception ex)
            {
                WriteLine("scoket: handleAccepted Error " + ex.Message, ConsoleColor.Red);
            }

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
        public static void WriteLine(string str, ConsoleColor color = ConsoleColor.Black)
        {
            Console.ForegroundColor = color;
            Console.WriteLine("[{0:MM-dd HH:mm:ss}] {1}", DateTime.Now, str);
        }
    }
}

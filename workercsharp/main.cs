using System;

namespace ff
{
    class FFMain
    {
        public static void WriteLine(string str, ConsoleColor color = ConsoleColor.Black)
        {
            Console.ForegroundColor = color;
            Console.WriteLine("[{0:MM-dd HH:mm:ss}] {1}", DateTime.Now, str);
        }
        public static void Main(string[] args)
        {
            FFAcceptor ffaceptor = new FFAcceptor(new SocketRecvHandler(onRecv), new SocketBrokenHandler(onBroken));
            ffaceptor.listen("*", 43210);

            WriteLine("scoket test", ConsoleColor.Red);

            FFSocket ffsocket = new FFScoketAsync(new SocketRecvHandler(onRecv2), new SocketBrokenHandler(onBroken2));
            ffsocket.connect("127.0.0.1", 43210);
            ffsocket.asyncSend("hello!");
            Console.ReadKey();
            ffsocket.asyncSend("hello!");
            Console.ReadKey();
            ffsocket.close();
            Console.ReadKey();
        }
        public static void onRecv(FFSocket socket, string strData){
            WriteLine("onRecv...." + strData);
            socket.asyncSend(strData);
        }

        public static void onRecv2(FFSocket socket, string strData){
            WriteLine("onRecv2...." + strData);
        }
        public static void onBroken(FFSocket socket){
            WriteLine("onBroken....");
        }
        public static void onBroken2(FFSocket socket){
            WriteLine("onBroken2....");
        }
    }
}

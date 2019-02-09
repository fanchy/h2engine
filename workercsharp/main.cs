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
            FFAcceptor ffaceptor = NetOps.listen("*", 43210, new SocketMsgHandler(onRecv), new SocketBrokenHandler(onBroken));

            WriteLine("scoket test", ConsoleColor.Red);

            FFSocket ffsocket = NetOps.connect("127.0.0.1", 43210, new SocketMsgHandler(onRecv2), new SocketBrokenHandler(onBroken2));

            NetOps.sendMsg(ffsocket, 10, "hello!");
            NetOps.sendMsg(ffsocket, 11, "hello2!");
            Console.ReadKey();
            NetOps.sendMsg(ffsocket, 11, "hello3!");
            Console.ReadKey();
            ffsocket.close();
            Console.ReadKey();
        }
        public static void onRecv(FFSocket ffsocket, Int16 cmd, string strData){
            Console.WriteLine("onRecv....{0}, {1}", strData, cmd);
            NetOps.sendMsg(ffsocket, cmd, strData);
        }

        public static void onRecv2(FFSocket ffsocket, Int16 cmd, string strData){
            Console.WriteLine("onRecv2....{0}, {1}", strData, cmd);
        }
        public static void onBroken(FFSocket ffsocket){
            WriteLine("onBroken....");
        }
        public static void onBroken2(FFSocket ffsocket){
            WriteLine("onBroken2....");
        }
    }
}

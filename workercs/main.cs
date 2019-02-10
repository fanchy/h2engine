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
            FFRpc ffrpc = new FFRpc("worker#0");
            if (ffrpc.open("127.0.0.1:43210") == false){
                Console.WriteLine("ffrpc open failed!");
            }
            Console.ReadKey();
        }
        public static void onRecv(FFSocket ffsocket, Int16 cmd, string strData){
            Console.WriteLine("onRecv....{0}, {1}", strData, cmd);
            FFNet.sendMsg(ffsocket, cmd, strData);
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

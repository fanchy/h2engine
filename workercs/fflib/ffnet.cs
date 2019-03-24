using System;
using System.Net.Sockets;
using System.Collections.Generic;
using Xunit;
namespace ff
{
    public delegate void SocketMsgHandler(IFFSocket ffsocket, Int16 cmd, string strData);
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
        public void HandleRecv(IFFSocket ffsocket, string strData){
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
        public void HandleBroken(IFFSocket ffsocket){
            m_funcBroken(ffsocket);
        }
    }

    struct TimerData
    {
        public FFTask task;
        public Int64 endms;
    };
    class FFNetContext
    {
        private TaskQueue m_taskQueue;
        private List<TimerData> m_taskTimer;
        private System.Timers.Timer m_timer;
        public FFNetContext()
        {
            m_taskQueue = new TaskQueue();
            m_taskTimer = new List<TimerData>();
            m_taskQueue.Run();
            m_timer = new System.Timers.Timer(50);
            m_timer.Elapsed += new System.Timers.ElapsedEventHandler(this.HandleTimeout);//到达时间的时候执行事件；
            m_timer.AutoReset = true;//设置是执行一次（false）还是一直执行(true)；
            m_timer.Enabled = true;//是否执行System.Timers.Timer.Elapsed事件；
        }
        public TaskQueue GetTaskQueue() { return m_taskQueue; }
        public void HandleTimeout(object source, System.Timers.ElapsedEventArgs e)
        {
            System.DateTime currentTime = DateTime.Now;
            Int64 n = ((Int64)currentTime.Second) * 1000 + currentTime.Millisecond;
            m_taskTimer.RemoveAll(data =>
            {
                if (n >= data.endms)
                {
                    m_taskQueue.Post(() =>
                    {
                        data.task();
                    });
                    return true;
                }
                return false;
            });
        }
        public void Timerout(int nms, FFTask task)
        {
            System.DateTime currentTime = DateTime.Now;
            TimerData data;
            data.task = task;
            data.endms = ((Int64)currentTime.Second) * 1000 + currentTime.Millisecond + nms;
            m_taskTimer.Add(data);
        }
    };
    class FFNet
    {
        public static FFNetContext gInstanceContext = null;
        public static FFNetContext GetContext()
        {
            if (gInstanceContext == null)
            {
                gInstanceContext = new FFNetContext();
            }
            return gInstanceContext;
        }
        public static TaskQueue GetTaskQueue() { return GetContext().GetTaskQueue();  }
        public static void Timerout(int nms, FFTask task)
        {
            GetContext().Timerout(nms, task);
        }

        public static bool Cleanup()
        {
            if (GetTaskQueue().IsRunning())
            {
                GetTaskQueue().Stop();
            }

            return true;
        }
        public static IFFSocket Connect(string host, SocketMsgHandler funcMsg, SocketBrokenHandler funcBroken){
            string[] strList = host.Split(":");
            if (strList.Length != 3)
            {
                return null;
            }
            string ip = strList[1];
            string[] ipList = ip.Split("//");
            if (ipList.Length == 2)
            {
                ip = ipList[1];
            }
            else
            {
                ip = ipList[0];
            }

            int port = int.Parse(strList[2]);
            SocketCtrl ctrl = new SocketCtrl(funcMsg, funcBroken);
            IFFSocket ffsocket = new FFScoketAsync(new SocketRecvHandler(ctrl.HandleRecv), new SocketBrokenHandler(ctrl.HandleBroken));
            if (ffsocket.Connect(ip, port)){
                return ffsocket;
            }
            return null;
        }
        public static FFAcceptor Listen(string host, SocketMsgHandler funcMsg, SocketBrokenHandler funcBroken){
            string[] strList = host.Split(":");
            if (strList.Length != 3)
            {
                return null;
            }
            string ip = strList[1];
            string[] ipList = ip.Split("//");
            if (ipList.Length == 2)
            {
                ip = ipList[1];
            }
            else
            {
                ip = ipList[0];
            }

            int port = int.Parse(strList[2]);
            SocketCtrl ctrl = new SocketCtrl(funcMsg, funcBroken);
            FFAcceptor ffacceptor = new FFAcceptor(new SocketRecvHandler(ctrl.HandleRecv), new SocketBrokenHandler(ctrl.HandleBroken));
            if (ffacceptor.Listen(ip, port)){
                return ffacceptor;
            }
            return null;
        }
        public static void SendMsg(IFFSocket ffsocket, Int16 cmd, string strData){
            int len = strData.Length;
            len = System.Net.IPAddress.HostToNetworkOrder(len);
            cmd = System.Net.IPAddress.HostToNetworkOrder(cmd);
            byte[] lenArray = BitConverter.GetBytes(len);
            byte[] cmdArray = BitConverter.GetBytes(cmd);
            byte[] resArray = new byte[2]{0, 0};
            string strRet = System.Text.Encoding.UTF8.GetString(lenArray, 0, lenArray.Length);
            //FFLog.Trace("sendMsg {0}, {1}", strRet.Length, strData.Length);
            strRet += System.Text.Encoding.UTF8.GetString(cmdArray, 0, cmdArray.Length);
            //FFLog.Trace("sendMsg {0}, {1}", strRet.Length, strData.Length);
            strRet += System.Text.Encoding.UTF8.GetString(resArray, 0, resArray.Length);
            //FFLog.Trace("sendMsg {0}, {1}", strRet.Length, strData.Length);
            strRet += strData;
            ffsocket.AsyncSend(strRet);
        }

        public static void SendMsg<T>(IFFSocket ffsocket, Int16 cmd, T reqMsg) where T : Thrift.Protocol.TBase{
            string strData = EncodeMsg<T>(reqMsg);
            SendMsg(ffsocket, cmd, strData);
        }
        public static string EncodeMsg<T>(T reqMsg) where T : Thrift.Protocol.TBase{
            var tmem = new Thrift.Transport.TMemoryBuffer();
            var proto = new Thrift.Protocol.TBinaryProtocol(tmem);
            //proto.WriteMessageBegin(new Thrift.Protocol.TMessage("ff::RegisterToBrokerReq", Thrift.Protocol.TMessageType.Call, 0));
            reqMsg.Write(proto);
            //proto.WriteMessageEnd();
            byte[] byteData = tmem.GetBuffer();
            string strRet = System.Text.Encoding.UTF8.GetString(byteData, 0, byteData.Length);
            return strRet;
        }
        public static bool DecodeMsg<T>(T reqMsg, string strData) where T : Thrift.Protocol.TBase{
            byte[] data = System.Text.Encoding.UTF8.GetBytes(strData);
            var tmem = new Thrift.Transport.TMemoryBuffer(data);
            var proto = new Thrift.Protocol.TBinaryProtocol(tmem);
            //var msgdef = new Thrift.Protocol.TMessage("ffthrift", Thrift.Protocol.TMessageType.Call, 0);
            //proto.ReadMessageBegin();
            reqMsg.Read(proto);
            //proto.ReadMessageEnd();
            return true;
        }
    }
}

using System;
using System.Net.Sockets;
using System.Collections.Generic;

using pb = global::Google.Protobuf;

namespace ff
{
    enum WorkerDef{
        OFFLINE_CMD = 0xFFFFFF
    }
    public delegate void CmdHandler(Int64 s, int cmd, byte[] data);
    public delegate void PbHandler<OBJ_TYPE, RET_TYPE>(OBJ_TYPE p, RET_TYPE t);//泛型委托
    public delegate void PbHandlerSession<RET_TYPE>(Int64 s, RET_TYPE t);//泛型委托
    public delegate object SessionID2Object(Int64 s, int cmd, byte[] data);//泛型委托
    
    public delegate object SessionOnEnterWorker(Int64 s, string fromWorker, byte[] data);//泛型委托
    public class CmdRegInfo
    {
        public CmdHandler cmdHandler;
        public string     cmdName;
    }
    public class FFWorker
    {
        private static FFWorker gInstance = null;
        public static FFWorker Instance() {
            if (gInstance == null){
                gInstance = new FFWorker();
            }
            return gInstance;
        }
        protected long m_nIDGenerator;
        protected int m_nWorkerIndex;//!这是第几个worker，现在只有一个worker
        protected string m_strWorkerName;
        protected EmptyMsgRet RPC_NONE;
        protected string m_strDefaultGate;
        protected FFRpc m_ffrpc;
        protected Dictionary<int, CmdRegInfo> m_dictCmd2Func;
        string[] m_listEnableClassNames;
        public FFWorker()
        {
            m_nIDGenerator = 0;
            m_nWorkerIndex = 0;
            m_strWorkerName = "";
            m_strDefaultGate = "gate#0";
            m_ffrpc = null;
            m_dictCmd2Func = new Dictionary<int, CmdRegInfo>();
            RPC_NONE = new EmptyMsgRet();
            m_listEnableClassNames = null;
        }
        public FFRpc GetRpc() { return m_ffrpc; }
        public SessionID2Object funcSessionID2Object {get; set;}
        public SessionOnEnterWorker funcSessionOnEnterWorker {get; set;}
        public bool Init(string strBrokerHost, int nWorkerIndex, string[] listEnableClassNames)
        {
            m_nWorkerIndex = nWorkerIndex;
            m_strWorkerName = string.Format("worker#{0}", m_nWorkerIndex);
            m_ffrpc = new FFRpc(m_strWorkerName);
            if (m_ffrpc.Open(strBrokerHost) == false)
            {
                FFLog.Error("worker ffrpc open failed!");
                return false;
            }

            m_ffrpc.Reg<RouteLogicMsgReq, EmptyMsgRet>(this.OnRouteLogicMsgReq);
            m_ffrpc.Reg<SessionOfflineReq, EmptyMsgRet>(this.OnSessionOfflineReq);
            m_ffrpc.Reg<SessionEnterWorkerReq, EmptyMsgRet>(this.OnSessionEnterWorkerReq);

            string err = Util.InitClassByNames(listEnableClassNames);
            if (err != ""){
                FFLog.Error("worker init failed mod="+err);
                return false;
            }

            m_listEnableClassNames = listEnableClassNames;

            return true;
        }
        public bool Cleanup()
        {
            Util.CleanupClassByNames(m_listEnableClassNames);
            return true;
        }
        public FFWorker BindHandler<T>(object cmdregO, PbHandlerSession<T> method) where T : pb::IMessage, new()
        {
            int cmdreg = (int)cmdregO;
            m_dictCmd2Func[cmdreg] = new CmdRegInfo(){
                cmdName = cmdregO.ToString(), 
                cmdHandler = (Int64 sid, int cmdSrc, byte[] data) =>
                {
                    T reqMsg = Util.Byte2Pb<T>(data);
                    method(sid, reqMsg);
                }
            };
            return this;
        }
        public FFWorker BindHandler<OBJ_TYPE, T>(object cmdregO, PbHandler<OBJ_TYPE, T> method) where T : pb::IMessage, new() where OBJ_TYPE: new()
        {
            int cmdreg = (int)cmdregO;
            m_dictCmd2Func[cmdreg] =  new CmdRegInfo(){
                cmdName = cmdregO.ToString(), 
                cmdHandler = (Int64 sid, int cmdSrc, byte[] data) =>
                {
                    int cmd = cmdSrc;
                    if ((cmd & 0x4000) != 0)
                    {
                        cmd &= ~(0x4000);
                    }
                    T reqMsg = Util.Byte2Pb<T>(data);
                    if (funcSessionID2Object != null){                   
                        object o = funcSessionID2Object(sid, cmdSrc, data);
                        if (o != null && o is OBJ_TYPE){
                            OBJ_TYPE obj = (OBJ_TYPE)o;
                            method(obj, reqMsg);
                        }
                    }
                }
            };
            return this;
        }
        
        public delegate void FuncHandlerOffline(Int64 s);
        public FFWorker BindOffline(FuncHandlerOffline method)
        {
            m_dictCmd2Func[(int)WorkerDef.OFFLINE_CMD] = new CmdRegInfo(){
                cmdName = "OFFLINE_CMD",
                cmdHandler = (Int64 sid, int cmdSrc, byte[] data) =>
                {
                    method(sid);
                }
            };
            return this;
        }
        public void SessionSendMsg<T>(Int64 nSessionID, Int16 nCmd, T pbMsgData) where T : pb::IMessage, new()
        {
            GateRouteMsgToSessionReq msgToSession = new GateRouteMsgToSessionReq() { Cmd = nCmd, Body = Util.Pb2Byte(pbMsgData) };
            msgToSession.SessionId.Add(nSessionID);
            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public void GateBroadcastMsg<T>(Int16 cmd, T pbMsgData) where T : pb::IMessage, new()
        {
            GateBroadcastMsgToSessionReq msgToSession = new GateBroadcastMsgToSessionReq() { Cmd = (Int16)cmd, Body = Util.Pb2Byte(pbMsgData) };
            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public void SessionMulticastMsg<T>(Int64[] listSessionID, Int16 nCmd, T pbMsgData) where T : pb::IMessage, new()
        {
            GateRouteMsgToSessionReq msgToSession = new GateRouteMsgToSessionReq() { Cmd = nCmd, Body = Util.Pb2Byte(pbMsgData) };
            foreach(var nSessionID in listSessionID)
            {
                msgToSession.SessionId.Add(nSessionID);
            }
            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public void SessionClose(Int64 nSessionID)
        {
            GateCloseSessionReq msgToSession = new GateCloseSessionReq() { SessionId = nSessionID };
            m_ffrpc.Call(m_strDefaultGate, msgToSession);
        }
        public void SessionChangeWorker(Int64 nSessionID, int nToWorkerIndex, byte[] data)
        {
            GateChangeLogicNodeReq msg = new GateChangeLogicNodeReq(){
                SessionId = nSessionID,  AllocWorker=string.Format("worker#{0}", nToWorkerIndex), ExtraData = data
            };
            m_ffrpc.Call(m_strDefaultGate, msg);
        }
        public EmptyMsgRet OnRouteLogicMsgReq(RouteLogicMsgReq reqMsg)
        {
            Int64 nBeginUs = DateTime.Now.Ticks / 10;
            int cmd = reqMsg.Cmd;
            if ((cmd & 0x4000) != 0)
            {
                cmd &= ~(0x4000);
            }
            Int64 nSessionID = reqMsg.SessionId;
            if (m_dictCmd2Func.ContainsKey(cmd) == false)
            {
                FFLog.Error(string.Format("worker cmd invalid! {0}", cmd));
                return RPC_NONE;
            }
            CmdRegInfo cmdRegInfo = m_dictCmd2Func[cmd];
            cmdRegInfo.cmdHandler(nSessionID, reqMsg.Cmd, reqMsg.Body);
            PerfMonitor.Instance().AddPerf(string.Format("cmd={0}", cmdRegInfo.cmdName), DateTime.Now.Ticks / 10 - nBeginUs);
            return RPC_NONE;
        }
        public EmptyMsgRet OnSessionOfflineReq(SessionOfflineReq reqMsg)
        {
            Int64 nBeginUs = DateTime.Now.Ticks / 10;
            Int64 nSessionID = reqMsg.SessionId;
            FFLog.Trace(string.Format("worker OnSessionOfflineReq! {0}", nSessionID));
            int cmd = (int)WorkerDef.OFFLINE_CMD;
            if (m_dictCmd2Func.ContainsKey(cmd) == false)
            {
                FFLog.Error(string.Format("worker cmd invalid! {0}", cmd));
                return RPC_NONE;
            }
            byte[] data = {};
            m_dictCmd2Func[cmd].cmdHandler(nSessionID, cmd, data);
            PerfMonitor.Instance().AddPerf("OnOffline", DateTime.Now.Ticks / 10 - nBeginUs);
            return RPC_NONE;
        }
        
        public EmptyMsgRet OnSessionEnterWorkerReq(SessionEnterWorkerReq reqMsg)
        {
            Int64 nBeginUs = DateTime.Now.Ticks / 10;
            Int64 nSessionID = reqMsg.SessionId;
            FFLog.Trace(string.Format("worker OnSessionEnterWorkerReq session={0}", nSessionID));

            this.funcSessionOnEnterWorker?.Invoke(nSessionID, reqMsg.FromWorker, reqMsg.ExtraData);
            PerfMonitor.Instance().AddPerf("OnEnterWorker", DateTime.Now.Ticks / 10 - nBeginUs);
            return RPC_NONE;
        }
    }
}

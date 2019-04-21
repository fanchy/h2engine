using System;
using System.Net.Sockets;
using System.Collections.Generic;

namespace ff
{
    struct ClientInfo
    {
        public long         sessionID;
        public IFFSocket    sockObj;
        public string       strAllocWorker;
    };
    class FFGate
    {
        public FFAcceptor m_acceptor;
        protected long m_nIDGenerator;
        protected int m_nGateIndex;//!这是第几个gate，现在只有一个gate，如果以后想要有多个gate，这个要被正确的赋值
        protected string m_strGateName;
        protected EmptyMsgRet m_msgEmpty;
        FFRpc m_ffrpc;
        protected Dictionary<Int64, ClientInfo> m_dictClients;
        public FFGate(string strName = "gate#0")
        {
            m_nIDGenerator = 0;
            m_nGateIndex = 0;
            m_strGateName = strName;
            m_ffrpc = null;
            m_dictClients = new Dictionary<Int64, ClientInfo>();
            m_msgEmpty = new EmptyMsgRet();
            m_acceptor = null;
        }
        public bool Open(string strBrokerHost, string strGateListenIpPort, int nGateIndex)
        {
            m_nGateIndex = nGateIndex;
            m_strGateName = string.Format("gate#{0}", m_nGateIndex);
            m_ffrpc = new FFRpc(m_strGateName);

            m_ffrpc.Reg<GateChangeLogicNodeReq, EmptyMsgRet>(this.ChangeSessionLogic);
            m_ffrpc.Reg<GateCloseSessionReq, EmptyMsgRet>(this.CloseSession);
            m_ffrpc.Reg<GateRouteMsgToSessionReq, EmptyMsgRet>(this.RouteMsgToSession);
            m_ffrpc.Reg<GateBroadcastMsgToSessionReq, EmptyMsgRet>(this.BroadcastMsgToSession);

            m_acceptor = FFNet.Listen(strGateListenIpPort, new SocketMsgHandler(HandleMsg), new SocketBrokenHandler(HandleBroken));
            if (m_acceptor != null)
            {
                FFLog.Trace(string.Format("FFGate open....{0} ok", strGateListenIpPort));
            }
            else
            {
                FFLog.Trace(string.Format("FFGate open....{0} failed", strGateListenIpPort));
            }
            return true;
        }
        //!切换worker
        public EmptyMsgRet ChangeSessionLogic(GateChangeLogicNodeReq reqMsg)
        {
            if (m_dictClients.ContainsKey(reqMsg.Session_id) == false)
            {
                return m_msgEmpty;
            }
            ClientInfo cinfo = m_dictClients[reqMsg.Session_id];
            SessionEnterWorkerReq msgEnter = new SessionEnterWorkerReq() { };
            
            msgEnter.From_worker = cinfo.strAllocWorker;
            cinfo.strAllocWorker = reqMsg.Alloc_worker;

            msgEnter.Session_id = reqMsg.Session_id;
            msgEnter.From_gate = m_strGateName;
            msgEnter.Session_ip = cinfo.sockObj.GetIP();

            msgEnter.To_worker = reqMsg.Alloc_worker;
            msgEnter.Extra_data = reqMsg.Extra_data;
            m_ffrpc.Call(reqMsg.Alloc_worker, msgEnter);
            return m_msgEmpty;
        }
        //! 关闭某个session socket
        public EmptyMsgRet CloseSession(GateCloseSessionReq reqMsg)
        {
            if (m_dictClients.ContainsKey(reqMsg.Session_id) == false)
            {
                return m_msgEmpty;
            }
            ClientInfo cinfo = m_dictClients[reqMsg.Session_id];
            cinfo.sockObj.Close();
            CleanupSession(cinfo, false);
            return m_msgEmpty;
        }
        public void CleanupSession(ClientInfo cinfo, bool closesend)
        {
            if (closesend)
            {
                SessionOfflineReq msg = new SessionOfflineReq() {Session_id= cinfo.sessionID};
                m_ffrpc.Call(cinfo.strAllocWorker, msg);
            }
            m_dictClients.Remove(cinfo.sessionID);
        }
        //! 转发消息给client
        public EmptyMsgRet RouteMsgToSession(GateRouteMsgToSessionReq reqMsg)
        {
            byte[] dataBody = Util.String2Byte(reqMsg.Body);
            foreach (var sessionID in reqMsg.Session_id)
            {
                if (m_dictClients.ContainsKey(sessionID) == false)
                {
                    continue;
                }
                ClientInfo cinfo = m_dictClients[sessionID];
                FFNet.SendMsg(cinfo.sockObj, (UInt16)reqMsg.Cmd, dataBody);
            }
            return m_msgEmpty;
        }
        //! 广播消息给所有的client
        public EmptyMsgRet BroadcastMsgToSession(GateBroadcastMsgToSessionReq reqMsg)
        {
            byte[] dataBody = Util.String2Byte(reqMsg.Body);
            foreach (var cinfo in m_dictClients.Values)
            {
                FFNet.SendMsg(cinfo.sockObj, (UInt16)reqMsg.Cmd, dataBody);
            }
            return m_msgEmpty;
        }
        public void HandleMsg(IFFSocket ffsocket, UInt16 cmd, byte[] strMsg)
        {
            var sessionData = ffsocket.GetSessionData();
            if (sessionData == null)//!first msg
            {
                string strDefaultWorker = "worker#0";
                if (m_ffrpc.IsExistNode(strDefaultWorker) == false)
                {
                    //ffsocket.Close();
                    FFLog.Error(string.Format("gate worker[{0}] not exist", strDefaultWorker));
                    FFNet.SendMsg(ffsocket, 0, Util.String2Byte("server is busy!0x0!"));
                    return;
                }
                Int64 sessionIDNew = ++m_nIDGenerator;
                ClientInfo cinfo = new ClientInfo() { sockObj = ffsocket, sessionID = sessionIDNew, strAllocWorker = strDefaultWorker };
                ffsocket.SetSessionData(sessionIDNew);
                m_dictClients[sessionIDNew] = cinfo;
                RouteLogicMsg(cinfo, cmd, strMsg, true);
                return;
            }
            Int64 sessionID = (Int64)sessionData;
            if (m_dictClients.ContainsKey(sessionID) == false)
            {
                return;
            }
            ClientInfo cinfo2 = m_dictClients[sessionID];
            RouteLogicMsg(cinfo2, cmd, strMsg, false);
        }
        public void HandleBroken(IFFSocket ffsocket)
        {
            foreach (KeyValuePair<Int64, ClientInfo> kvp in m_dictClients)
            {
                if (kvp.Value.sockObj == ffsocket)
                {
                    CleanupSession(kvp.Value, true);
                    break;
                }
            }
        }
        //! 逻辑处理,转发消息到logic service
        public void RouteLogicMsg(ClientInfo cinfo, UInt16 cmd, byte[] strMsg, bool first)
        {
            RouteLogicMsgReq msg = new RouteLogicMsgReq() { Session_id= cinfo.sessionID, Cmd= (Int16)cmd, Body= Util.Byte2String(strMsg)};
            if (first)
            {
                msg.Session_ip = cinfo.sockObj.GetIP();
            }
            m_ffrpc.Call(cinfo.strAllocWorker, msg);
        }
    }
}

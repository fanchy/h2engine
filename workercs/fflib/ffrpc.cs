using System;
using System.Net.Sockets;
using System.Collections.Generic;

namespace ff
{

    enum FFRPC_CMD
    {
        BROKER_TO_CLIENT_MSG = 1,
        //!新版本*********
        REGISTER_TO_BROKER_REQ,
        REGISTER_TO_BROKER_RET,
        BROKER_ROUTE_MSG,
        SYNC_CLIENT_REQ, //! 同步客户端的请求，如python,php
    };
    //! 各个节点的类型
    enum FFRPC_NODE_TYPE
    {
        UNKNOWN_NODE = 0,
        MASTER_BROKER, //! 每个区服的主服务器
        RPC_NODE,      //! rpc节点
        SYNC_CLIENT_NODE, //! 同步调用client
    };
    public delegate void FFRpcFunc(BrokerRouteMsgReq reqMsg);
    public delegate RET_TYPE TUserFunc<MSG_TYPE, RET_TYPE>(MSG_TYPE t);//泛型委托
    public delegate void TCallBack<RET_TYPE>(RET_TYPE t);//泛型委托
    class FFRpc
    {
        protected IFFSocket m_socketBroker;
        protected string m_strServiceName;
        protected string m_strBrokerHost;
        protected long m_nNodeID;
        protected RegisterToBrokerRet m_brokerData;
        protected Dictionary<string/* msg name */, FFRpcFunc> m_dictFuncs;
        protected Dictionary<long, FFRpcFunc> m_dictCallBack;
        protected long m_nIDGenerator;
        public FFRpc(string strName) {
            m_strServiceName = strName;
            m_strBrokerHost = "tcp://127.0.0.1:43210";
            m_brokerData = new RegisterToBrokerRet() { NodeId = 0, RegisterFlag = 0, Service2nodeId = new Dictionary<string, long>() };
            m_dictFuncs = new Dictionary<string/* msg name */, FFRpcFunc>();
            m_dictCallBack = new Dictionary<long, FFRpcFunc>();
            m_nIDGenerator = 0;
        }
        public TaskQueue GetTaskQueue() { return FFNet.GetTaskQueue(); }
        public bool Open(string strBrokerCfg) {
            if (strBrokerCfg.Length > 0)
            {
                m_strBrokerHost = strBrokerCfg;
            }

            FFLog.Trace(string.Format("ffrpc open....{0} begin", m_strBrokerHost));
            if (!ConnectToBroker()) {
                return false;
            }
            return true;
        }
        public bool ConnectToBroker() {
            m_socketBroker = FFNet.Connect(m_strBrokerHost, new SocketMsgHandler(HandleMsg), new SocketBrokenHandler(HandleBroken));
            if (null == m_socketBroker) {
                return false;
            }
            RegisterToBrokerReq reqMsg = new RegisterToBrokerReq { NodeType = 2, StrServiceName = m_strServiceName };

            FFNet.SendMsg(m_socketBroker, (UInt16)FFRPC_CMD.REGISTER_TO_BROKER_REQ, reqMsg);
            return true;
        }
        public bool Close() {
            if (m_socketBroker != null)
            {
                m_socketBroker.Close();
                m_socketBroker = null;
            }
            return true;
        }
        public string Type2Name<MSG_TYPE>(MSG_TYPE msgData)
        {
            Type t = msgData.GetType();
            string strType = t.ToString();
            string[] strList = strType.Split(".");
            string msgname = strList[strList.Length - 1];
            return msgname;
        }
        public bool IsExistNode(string strServiceName)
        {
            if (m_brokerData.Service2nodeId.ContainsKey(strServiceName))
            {
                return true;
            }
            return false;
        }
        public FFRpc Reg<MSG_TYPE, RET_TYPE>(TUserFunc<MSG_TYPE, RET_TYPE> funcUser) 
            where MSG_TYPE : Thrift.Protocol.TBase, new()
            where RET_TYPE : Thrift.Protocol.TBase, new()
        {
            MSG_TYPE tmpData = new MSG_TYPE();
            string msgname = Type2Name(tmpData);
            FFLog.Trace(string.Format("ffrpc.FFRpc Reg....msgname={0}", msgname));

            m_dictFuncs[msgname] = (BrokerRouteMsgReq reqMsg) =>
            {
                MSG_TYPE msgData = new MSG_TYPE();
                FFNet.DecodeMsg(msgData, reqMsg.Body);
                RET_TYPE retMsg = funcUser(msgData);
                if (reqMsg.CallbackId != 0)
                {
                    reqMsg.DestNodeId = reqMsg.FromNodeId;
                    reqMsg.DestServiceName = "";
                    reqMsg.Body = FFNet.EncodeMsg(retMsg);//Util.Byte2String(
                    SendToDestNode(reqMsg);
                }
            };

            return this;
        }
        public bool Call<MSG_TYPE>(string strServiceName, MSG_TYPE msgData)
            where MSG_TYPE : Thrift.Protocol.TBase, new()
        {
            if (!m_brokerData.Service2nodeId.ContainsKey(strServiceName))
            {
                FFLog.Error(string.Format("ffrpc.Call servervice:{0} not exist", strServiceName));
                return false;
            }
            BrokerRouteMsgReq reqMsg = new BrokerRouteMsgReq() { CallbackId = 0, Errinfo = ""};
            reqMsg.DestNodeId = m_brokerData.Service2nodeId[strServiceName];
            reqMsg.DestServiceName = strServiceName;
            reqMsg.Body = FFNet.EncodeMsg(msgData);//Util.Byte2String(
            reqMsg.DestMsgName = Type2Name(msgData);
            SendToDestNode(reqMsg);
            return true;
        }
        public bool Call<MSG_TYPE, RET_TYPE>(string strServiceName, MSG_TYPE msgData, TCallBack<RET_TYPE> callback)
            where MSG_TYPE : Thrift.Protocol.TBase, new()
            where RET_TYPE : Thrift.Protocol.TBase, new()
        {
            if (!m_brokerData.Service2nodeId.ContainsKey(strServiceName))
            {
                FFLog.Error(string.Format("ffrpc.Call servervice:{0} not exist", strServiceName));
                RET_TYPE retMsg = new RET_TYPE();
                callback(retMsg);
                return false;
            }
            BrokerRouteMsgReq reqMsg = new BrokerRouteMsgReq() { CallbackId = 0, Errinfo = "" };
            reqMsg.DestNodeId = m_brokerData.Service2nodeId[strServiceName];
            reqMsg.DestServiceName = strServiceName;
            reqMsg.Body = FFNet.EncodeMsg(msgData);//Util.Byte2String(
            reqMsg.DestMsgName = Type2Name(msgData);
            reqMsg.CallbackId = ++m_nIDGenerator;
            m_dictCallBack[reqMsg.CallbackId] = (BrokerRouteMsgReq dataMsg) =>
            {
                RET_TYPE retMsg = new RET_TYPE();
                FFNet.DecodeMsg(retMsg, dataMsg.Body);
                callback(retMsg);
            };
            SendToDestNode(reqMsg);
            return true;
        }
        public void HandleMsg(IFFSocket ffsocket, UInt16 cmd, byte[] strMsg){
            //FFLog.Trace(string.Format("ffrpc.FFRpc handleMsg....{0}, {1} [{2}]", cmd, strMsg.Length, System.Threading.Thread.CurrentThread.ManagedThreadId.ToString()));
            try
            {
                switch ((FFRPC_CMD)cmd)
                {
                    case FFRPC_CMD.REGISTER_TO_BROKER_RET:
                        {
                            FFNet.DecodeMsg(m_brokerData, strMsg);
                            FFLog.Trace(string.Format("ffrpc.handleMsg..REGISTER_TO_BROKER_RET..{0},{1},{2}", m_strServiceName, m_brokerData.NodeId, m_brokerData.RegisterFlag));
                            if (m_brokerData.RegisterFlag == 1)
                            {
                                m_nNodeID = m_brokerData.NodeId;//! -1表示注册失败，0表示同步消息，1表示注册成功
                            }
                        }break;
                    case FFRPC_CMD.BROKER_TO_CLIENT_MSG:
                        {
                            
                            try
                            {
                                BrokerRouteMsgReq reqMsg = new BrokerRouteMsgReq();
                                FFNet.DecodeMsg(reqMsg, strMsg);
                                if (reqMsg.Errinfo == null)
                                    reqMsg.Errinfo = "";
                                FFLog.Trace(string.Format("ffrpc.BROKER_TO_CLIENT_MSG msgname={0}", reqMsg.DestMsgName));
                                if (reqMsg.Errinfo.Length > 0)
                                {
                                    FFLog.Error(string.Format("FFRpc::handleRpcCallMsg error={0}", reqMsg.Errinfo));
                                    if (reqMsg.CallbackId == 0)
                                    {
                                        return;
                                    }
                                }

                                if (reqMsg.DestServiceName.Length > 0)
                                {
                                    if (!m_dictFuncs.ContainsKey(reqMsg.DestMsgName))
                                    {
                                        reqMsg.Errinfo = "interface named " + reqMsg.DestMsgName + " not found in rpc";
                                        FFLog.Error(string.Format("FFRpc::handleRpcCallMsg error={0}", reqMsg.Errinfo));
                                        reqMsg.DestNodeId = reqMsg.FromNodeId;
                                        reqMsg.DestServiceName = "";
                                        SendToDestNode(reqMsg);
                                        return;
                                    }
                                    FFRpcFunc destFunc = m_dictFuncs[reqMsg.DestMsgName];
                                    destFunc(reqMsg);
                                }
                                else
                                {
                                    if (!m_dictCallBack.ContainsKey(reqMsg.CallbackId))
                                    {
                                        return;
                                    }
                                    FFRpcFunc destFunc = m_dictCallBack[reqMsg.CallbackId];
                                    destFunc(reqMsg);
                                }
                            }
                            catch (Exception ex)
                            {
                                FFLog.Error("ffrpc handleMsg" + ex.Message);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
            catch (Exception ex)
            {
                FFLog.Error("ffprc.Error:" + ex.Message);
            }
}
        public void HandleBroken(IFFSocket ffsocket){

        }
        public void SendToDestNode(BrokerRouteMsgReq retMsg)
        {
            retMsg.FromNodeId = m_nNodeID;
            FFNet.SendMsg(m_socketBroker, (UInt16)FFRPC_CMD.BROKER_ROUTE_MSG, retMsg);
        }
    }
}

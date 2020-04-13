namespace cpp ff

struct SessionOfflineReq{
    2:i64 sessionId                  = 0
}

struct RouteLogicMsgReq{
    1:i64 sessionId                  = 0
    2:i16 cmd                         = 0
    3:binary body
    4:string sessionIp
}

struct GateChangeLogicNodeReq{
    1:i64 sessionId                  = 0
    2:string allocWorker
    3:binary extraData
}

struct GateCloseSessionReq{
    1:i64 sessionId                  = 0
}

struct GateRouteMsgToSessionReq{
    1:list<i64> sessionId            = 0
    2:i16 cmd                         = 0
    3:binary body
}

struct GateBroadcastMsgToSessionReq{
    1:i16 cmd                         = 0
    2:binary body
}
struct WorkerCallMsgReq{
    1:i16 cmd                         = 0
    2:binary body
}
struct WorkerCallMsgRet{
    1:string err;
    2:string msgType;
    3:binary body;
}

struct BrokerRouteMsgReq{
    2:string destServiceName;
    3:string destMsgName;
    4:i64 destNodeId;
    6:i64 fromNodeId;
    7:i64 callbackId;
    8:binary body;
    9:string errinfo;
}
struct RegisterToBrokerReq{
    1:i32 nodeType;
    2:string strServiceName;
}
struct RegisterToBrokerRet{
    1:i16 registerFlag;
    2:i64 nodeId;
    3:map<string, i64>  service2nodeId;
}

struct EmptyMsgRet{
}

struct SessionEnterWorkerReq{
    1:i64 sessionId;
    2:string sessionIp;
    3:string fromGate;
    4:string fromWorker;
    5:string toWorker;
    6:binary extraData;
}

struct VarData{
    1:map<i32, i64>             intData;
    2:map<i32, string>          strData;
    3:map<i32, map<i32, i64> >  mapData;
}



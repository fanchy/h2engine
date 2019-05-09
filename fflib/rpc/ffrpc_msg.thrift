namespace cpp ff

struct SessionOfflineReq{
    2:i64 session_id                  = 0
}

struct RouteLogicMsgReq{
    1:i64 session_id                  = 0
    2:i16 cmd                         = 0
    3:string body
    4:string session_ip
}

struct GateChangeLogicNodeReq{
    1:i64 session_id                  = 0
    2:string alloc_worker
    3:string cur_group_name
    4:string dest_group_name
    5:string extra_data
}

struct GateCloseSessionReq{
    1:i64 session_id                  = 0
}

struct GateRouteMsgToSessionReq{
    1:list<i64> session_id            = 0
    2:i16 cmd                         = 0
    3:string body
}

struct GateBroadcastMsgToSessionReq{
    1:i16 cmd                         = 0
    2:string body
}
struct WorkerCallMsgReq{
    1:i16 cmd                         = 0
    2:string body
}
struct WorkerCallMsgRet{
    1:string err;
    2:string msg_type;
    3:string body;
}

struct BrokerRouteMsgReq{
    2:string dest_service_name;
    3:string dest_msg_name;
    4:i64 dest_node_id;
    6:i64 from_node_id;
    7:i64 callback_id;
    8:string body;
    9:string err_info;
}
struct RegisterToBrokerReq{
    1:i32 node_type;
    2:string service_name;
}
struct RegisterToBrokerRet{
    1:i16 register_flag;
    2:i64 node_id;
    3:map<string, i64>  service2node_id;
}

struct EmptyMsgRet{
}

struct SessionEnterWorkerReq{
    1:i64 session_id;
    2:string session_ip;
    3:string from_gate;
    4:string from_worker;
    5:string to_worker;
    6:string extra_data;
}

namespace cpp ffrpc_msg

struct session_offline_in_t{      
    2:i64 session_id                  = 0             
}
struct session_offline_out_t{
}

struct routeLogicMsg_in_t{
    1:i64 session_id                  = 0
    2:i16 cmd                         = 0
    3:string body
    4:string session_ip
}
struct routeLogicMsg_out_t{
}

struct gate_change_logic_node_in_t{
    1:i64 session_id                  = 0
    2:string alloc_worker   
    3:string cur_group_name           
    4:string dest_group_name
    5:string extra_data
}
struct gate_change_logic_node_out_t{
}
struct gate_closeSession_in_t{
    1:i64 session_id                  = 0
}
struct gate_closeSession_out_t{
}
struct gate_routeMmsgToSession_in_t{
    1:list<i64> session_id            = 0
    2:i16 cmd                         = 0
    3:string body
}
struct gate_routeMmsgToSession_out_t{
}

struct gate_broadcastMsgToSession_in_t{
    1:i16 cmd                         = 0
    2:string body
}
struct gate_broadcastMsgToSession_out_t{
}
struct worker_call_msg_in_t{
    1:i16 cmd                         = 0
    2:string body
}
struct worker_call_msg_out_t{
    1:string err;
    2:string msg_type;
    3:string body;
}

struct broker_route_msg_in_t{
    1:string dest_namespace;
    2:string dest_service_name;
    3:string dest_msg_name;
    4:i64 dest_node_id;
    5:string from_namespace;
    6:i64 from_node_id;
    7:i64 callback_id;
    8:string body;
    9:string err_info;
}
struct register_to_broker_in_t{
    1:i32 node_type;
    2:string host;
    3:string service_name;
    4:i64 node_id;
    5:string reg_namespace;
    6:i64 bind_broker_id;
}
struct register_to_broker_out_t{
    1:i16 register_flag;
    2:i64 node_id;
    3:map<string, i64>  service2node_id;
    4:map<string, i64>  slave_broker_data;
    5:map<i64, i64>     rpc_bind_broker_info;
    6:list<string>      reg_namespace_list;
}

struct empty_ret_msg{
}

struct session_enter_worker_in_t{
    1:i64 session_id;
    2:string session_ip;
    3:string from_gate;
    4:string from_worker;
    5:string to_worker;
    6:string extra_data;
}
struct session_enter_worker_out_t{
}

#ifndef _FF_RPC_OPS_H_
#define _FF_RPC_OPS_H_

#include <assert.h>
#include <string>

#include "net/socket.h"
#include "base/fftype.h"
#include "base/singleton.h"


#include <thrift/FFThrift.h>

#include "rpc/msg_def/ffrpc_msg_types.h"
#include "rpc/msg_def/ffrpc_msg_constants.h"

namespace apache
{
    namespace thrift{
        namespace protocol
        {
            class TProtocol;
        }
    }
}
namespace ff
{

//! 各个节点的类型
enum nodeType_e
{
    UNKNOWN_NODE = 0,
    MASTER_BROKER, //! 每个区服的主服务器
    RPC_NODE,      //! rpc节点
    SYNC_CLIENT_NODE, //! 同步调用client
};

#define BROKER_MASTER_nodeId   0
template<typename CLASS_TYPE, typename MSG_TYPE>
struct MsgHandleUtil
{
    static MsgHandleUtil<CLASS_TYPE, MSG_TYPE>  bind(int(CLASS_TYPE::*f)(SocketObjPtr, MSG_TYPE&), CLASS_TYPE* p){
        MsgHandleUtil<CLASS_TYPE, MSG_TYPE> ret(f, p);
        return ret;
    }
    MsgHandleUtil(int(CLASS_TYPE::*f)(SocketObjPtr, MSG_TYPE&), CLASS_TYPE* p):obj(p), func(f){}
    void operator()(SocketObjPtr s, const std::string& data){
        MSG_TYPE msg;
        FFThrift::DecodeFromString(msg, data);
        (obj->*(func))(s, msg);
    }
    CLASS_TYPE* obj;
    int (CLASS_TYPE::*func)(SocketObjPtr, MSG_TYPE&);
};
typedef Function<void(SocketObjPtr, const std::string&)> MsgCallBack;

enum ffrpc_cmd_def_e
{
    BROKER_TO_CLIENT_MSG = 1,
    //!新版本*********
    REGISTER_TO_BROKER_REQ,
    REGISTER_TO_BROKER_RET,
    BROKER_ROUTE_MSG,
    SYNC_CLIENT_REQ, //! 同步客户端的请求，如python,php
};

//! 用于broker 和 rpc 在内存间投递消息
class FFRpc;
class FFBroker;
struct FFRpcMemoryRoute
{
    struct dest_node_info_t
    {
        dest_node_info_t(FFRpc* rpc_ = NULL):
            ffrpc(rpc_)
        {}
        FFRpc*        ffrpc;
    };

    typedef std::map<uint64_t/*node id*/, dest_node_info_t> node_info_map_t;
    int addNode(uint64_t nodeId_, FFRpc* ffrpc_)
    {
        LockGuard lock(m_mutex);
        node_info_map_t tmp_data = m_node_info.get_data();
        for (node_info_map_t::iterator it = tmp_data.begin(); it != tmp_data.end(); ++it)
        {
            if (it->second.ffrpc == ffrpc_)
            {
                tmp_data.erase(it);
                break;
            }
        }
        tmp_data[nodeId_].ffrpc = ffrpc_;
        m_node_info.update_data(tmp_data);
        return 0;
    }
    int delNode(uint64_t nodeId_)
    {
        LockGuard lock(m_mutex);
        node_info_map_t tmp_data = m_node_info.get_data();
        tmp_data.erase(nodeId_);
        m_node_info.update_data(tmp_data);
        return 0;
    }
    FFRpc* getRpc(uint64_t nodeId_)
    {
        const node_info_map_t& tmp_data = m_node_info.get_data();
        node_info_map_t::const_iterator it = tmp_data.find(nodeId_);
        if (it != tmp_data.end())
        {
            return it->second.ffrpc;
        }
        return NULL;
    }
    SafeStl<node_info_map_t> m_node_info;
    Mutex                     m_mutex;
};

}

#endif

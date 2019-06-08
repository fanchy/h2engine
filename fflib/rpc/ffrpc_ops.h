
#ifndef _FF_RPC_OPS_H_
#define _FF_RPC_OPS_H_

#include <assert.h>
#include <string>


#include "base/ffslot.h"
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

class RPCResponser
{
public:
    virtual ~RPCResponser(){}
    virtual void response(const std::string& msg_name_,  uint64_t destNodeId_,
                          int64_t callbackId_, const std::string& body_, std::string err = "") = 0;
};

class SlotReqArg: public FFSlot::CallBackArg
{
public:
    SlotReqArg(const std::string& s_, uint64_t n_, int64_t cb_id_, std::string errinfo_, RPCResponser* p):
        body(s_),
        destNodeId(n_),
        callbackId(cb_id_),
        errinfo(errinfo_),
        responser(p)
    {}
    SlotReqArg(){}
    SlotReqArg(const SlotReqArg& src):
        body(src.body),
        destNodeId(src.destNodeId),
        callbackId(src.callbackId),
        errinfo(src.errinfo),
        responser(src.responser)
    {}
    SlotReqArg& operator=(const SlotReqArg& src)
    {
        body = src.body,
        destNodeId = src.destNodeId;
        callbackId = src.callbackId;
        errinfo = src.errinfo;
        responser = src.responser;
        return *this;
    }
    virtual int type()
    {
        return TYPEID(SlotReqArg);
    }
    std::string          body;
    uint64_t             destNodeId;//! 请求来自于那个node id
    int64_t              callbackId;//! 回调函数标识id
    std::string          errinfo;
    RPCResponser*        responser;
};



class null_type_t//: public FFMsg<null_type_t>
{
    void encode(){}
    void decode(){}
};

template<typename IN_T, typename OUT_T = null_type_t>
struct RPCReq
{
    RPCReq():
        destNodeId(0),
        callbackId(0),
        responser(NULL)
    {}
    bool error() const { return errinfo.empty() == false; }
    const std::string& errorMsg() const { return errinfo; }

    void response(OUT_T& out_)
    {
        if (0 != callbackId)
        {
        	responser->response(TYPE_NAME(OUT_T), destNodeId, callbackId, FFThrift::EncodeAsString(out_));
            callbackId = 0;
		}

    }
    IN_T                 msg;
    uint64_t             destNodeId;
    int64_t              callbackId;
    RPCResponser*        responser;
    std::string          errinfo;
};



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
        dest_node_info_t(FFRpc* rpc_ = NULL, FFBroker* broker_ = NULL):
            ffrpc(rpc_),
            ffbroker(broker_)
        {}
        FFRpc*        ffrpc;
        FFBroker*     ffbroker;
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
    int addNode(uint64_t nodeId_, FFBroker* ffbroker_)
    {
        LockGuard lock(m_mutex);
        node_info_map_t tmp_data = m_node_info.get_data();
        for (node_info_map_t::iterator it = tmp_data.begin(); it != tmp_data.end(); ++it)
        {
            if (it->second.ffbroker == ffbroker_)
            {
                tmp_data.erase(it);
                break;
            }
        }
        tmp_data[nodeId_].ffbroker = ffbroker_;
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
    FFBroker* getBroker(uint64_t nodeId_)
    {
        const node_info_map_t& tmp_data = m_node_info.get_data();
        node_info_map_t::const_iterator it = tmp_data.find(nodeId_);
        if (it != tmp_data.end())
        {
            return it->second.ffbroker;
        }
        return NULL;
    }
    uint64_t getBrokerInMem()
    {
        const node_info_map_t& tmp_data = m_node_info.get_data();
        node_info_map_t::const_iterator it = tmp_data.begin();
        for (; it != tmp_data.end(); ++it)
        {
            if (NULL != it->second.ffbroker)
            {
                return it->first;
            }
        }
        return 0;
    }
    SafeStl<node_info_map_t> m_node_info;
    Mutex                     m_mutex;
};

}

#endif

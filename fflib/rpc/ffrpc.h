//! 消息发送管理
#ifndef _FF_FFRPC_H_
#define _FF_FFRPC_H_

#include <string>
#include <map>
#include <vector>
#include <set>


#include "net/msg_handler.h"
#include "base/task_queue.h"
#include "base/ffslot.h"
#include "net/codec.h"
#include "base/thread.h"
#include "rpc/ffrpc_ops.h"
#include "net/msg_sender.h"
#include "base/timer_service.h"
#include "base/arg_helper.h"

namespace ff {
class FFRpc: public MsgHandler, RPCResponser
{
    struct SessionData;
public:
    FFRpc(std::string service_name_ = "");
    virtual ~FFRpc();

    int open(ArgHelper& arg_);
    int open(const std::string& broker_addr);
    int close();

    //! 处理连接断开
    int handleBroken(SocketObjPtr sock_);
    //! 处理消息
    int handleMsg(const Message& msg_, SocketObjPtr sock_);
    TaskQueue* getTaskQueue();

    //! 注册接口
    template <typename R, typename IN_T, typename OUT_T>
    FFRpc& reg(R (*)(RPCReq<IN_T, OUT_T>&));
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
    FFRpc& reg(R (CLASS_TYPE::*)(RPCReq<IN_T, OUT_T>&), CLASS_TYPE* obj);

    //! 调用远程的接口
    template <typename T>
    int call(const std::string& name_, T& req_, FFSlot::FFCallBack* callback_ = NULL);
    //! 调用其他broker master 组的远程的接口
    template <typename T>
    int call(const std::string& namespace_, const std::string& name_, T& req_, FFSlot::FFCallBack* callback_ = NULL);

#ifdef FF_ENABLE_PROTOCOLBUF
    template <typename R, typename IN_T, typename OUT_T>
    FFRpc& reg(R (*)(RPCReqPB<IN_T, OUT_T>&));
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
    FFRpc& reg(R (CLASS_TYPE::*)(RPCReqPB<IN_T, OUT_T>&), CLASS_TYPE* obj);
    //! 调用远程的接口
    template <typename T>
    int callPB(const std::string& name_, T& req_, FFSlot::FFCallBack* callback_ = NULL);
    //! 调用其他broker master 组的远程的接口
    template <typename T>
    int callPB(const std::string& namespace_, const std::string& name_, T& req_, FFSlot::FFCallBack* callback_ = NULL);
#endif

    //! call 接口的实现函数，call会将请求投递到该线程，保证所有请求有序
    int docall(const std::string& service_name_, const std::string& msg_name_, const std::string& body_, FFSlot::FFCallBack* callback_);
    //! 调用接口后，需要回调消息给请求者
    virtual void response(const std::string& msg_name_,  uint64_t dest_node_id_,
                          int64_t callback_id_, const std::string& body_, std::string err_info = "");
    //! 通过node id 发送消息给broker
    void sendToDestNode(const std::string& service_name_, const std::string& msg_name_,
                           uint64_t dest_node_id_, int64_t callback_id_, const std::string& body_, std::string error_info = "");

    //! 定时重连 broker master
    void timerReconnectBroker();

    Timer& getTimer() { return m_timer; }

    //! 判断某个service是否存在
    bool isExist(const std::string& service_name_);

    //!获取broker socket
    SocketObjPtr getBrokerSocket();
    //! 新版 调用消息对应的回调函数
    int handleRpcCallMsg(BrokerRouteMsgReq& msg_, SocketObjPtr sock_);
    //! 注册接口【支持动态的增加接口】
    void reg(const std::string& name_, FFSlot::FFCallBack* func_);

    const std::string& getHostCfg(){ return m_host; }
    //!获取某一类型的service
    std::vector<std::string> getServicesLike(const std::string& token);
    std::string getServicesById(uint64_t dest_node_id_);
    bool isOK() {return m_runing;}
private:
    SocketObjPtr connectToBroker(const std::string& host_, uint32_t node_id_);

    //! 新版实现
    //! 处理注册,
    int handleBrokerRegResponse(RegisterToBrokerRet& msg_, SocketObjPtr sock_);

public:
    bool                                            m_runing;
private:
    std::string                                     m_host;
    std::string                                     m_service_name;//! 注册的服务名称
    uint64_t                                        m_node_id;     //! 通过注册broker，分配的node id

    SharedPtr<TaskQueue>                            m_tq;
    Timer                                           m_timer;

    FFSlot                                          m_ffslot;//! 注册broker 消息的回调函数
    FFSlot                                          m_ffslot_interface;//! 通过reg 注册的接口会暂时的存放在这里
    FFSlot                                          m_ffslot_callback;//!
    SocketObjPtr                                    m_master_broker_sock;

    //!ff new impl
    RegisterToBrokerRet                             m_broker_data;
};

//! 注册接口
template <typename R, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (*func_)(RPCReq<IN_T, OUT_T>&))
{
    m_ffslot_interface.bind(TYPE_NAME(IN_T), FFRpcOps::genCallBack(func_));
    return *this;
}
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&), CLASS_TYPE* obj)
{
    m_ffslot_interface.bind(TYPE_NAME(IN_T), FFRpcOps::genCallBack(func_, obj));
    return *this;
}

struct FFRpc::SessionData
{
    SessionData(uint32_t n = 0):
        node_id(n)
    {}
    uint32_t getNodeId() { return node_id; }
    uint32_t node_id;
};

//! 调用远程的接口
template <typename T>
int FFRpc::call(const std::string& name_, T& req_, FFSlot::FFCallBack* callback_)
{
    getTaskQueue()->post(TaskBinder::gen(&FFRpc::docall, this, name_, TYPE_NAME(T), FFThrift::EncodeAsString(req_), callback_));
    return 0;
}

//! 调用其他broker master 组的远程的接口
template <typename T>
int FFRpc::call(const std::string& namespace_, const std::string& name_, T& req_, FFSlot::FFCallBack* callback_)
{
    if (namespace_.empty())
    {
        return this->call(name_, req_, callback_);
    }
    return 0;
}

#ifdef FF_ENABLE_PROTOCOLBUF
template <typename R, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (*func_)(RPCReqPB<IN_T, OUT_T>&))
{
    m_ffslot_interface.bind(TYPE_NAME(IN_T), FFRpcOps::genCallBack(func_));
    return *this;
}
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&), CLASS_TYPE* obj)
{
    m_ffslot_interface.bind(TYPE_NAME(IN_T), FFRpcOps::genCallBack(func_, obj));
    return *this;
}


//! 调用远程的接口
template <typename T>
int FFRpc::callPB(const std::string& name_, T& req_, FFSlot::FFCallBack* callback_)
{
    std::string ret;
    req_.SerializeToString(&ret);
    getTaskQueue()->post(TaskBinder::gen(&FFRpc::docall, this, name_, TYPE_NAME(T), ret, callback_));
    return 0;
}

//! 调用其他broker master 组的远程的接口
template <typename T>
int FFRpc::callPB(const std::string& namespace_, const std::string& name_, T& req_, FFSlot::FFCallBack* callback_)
{
    if (namespace_.empty())
    {
        return this->callPB(name_, req_, callback_);
    }
    else{
        std::string ret;
        req_.SerializeToString(&ret);
        getTaskQueue()->post(TaskBinder::gen(&FFRpc::bridgeDocall, this, namespace_, name_, TYPE_NAME(T), ret, callback_));
    }
    return 0;
}
#endif
}
#endif

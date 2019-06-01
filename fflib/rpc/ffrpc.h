//! 消息发送管理
#ifndef _FF_FFRPC_H_
#define _FF_FFRPC_H_

#include <string>
#include <map>
#include <vector>
#include <set>

#include "base/task_queue.h"
#include "base/ffslot.h"
#include "base/thread.h"
#include "rpc/ffrpc_ops.h"
#include "net/msg_sender.h"
#include "base/timer_service.h"
#include "base/arg_helper.h"

namespace ff {

typedef Function<void(const std::string&)> FFRpcCallBack;

class FFRpc: public RPCResponser
{
    struct SessionData;
public:
    typedef Function<void(SocketObjPtr, const std::string&)> MsgCallBack;

    FFRpc(std::string strServiceName_ = "");
    virtual ~FFRpc();

    int open(ArgHelper& arg_);
    int open(const std::string& broker_addr);
    int close();

    //! 处理连接断开
    int handleBroken(SocketObjPtr sock_);
    //! 处理消息
    int handleMsg(const Message& msg_, SocketObjPtr sock_);
    void handleSocketProtocol(SocketObjPtr sock_, int eventType, const Message& msg_);
    TaskQueue& getTaskQueue();

    //! 注册接口
    template <typename R, typename IN_T, typename OUT_T>
    FFRpc& reg(R (*)(RPCReq<IN_T, OUT_T>&));
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
    FFRpc& reg(R (CLASS_TYPE::*)(RPCReq<IN_T, OUT_T>&), CLASS_TYPE* obj);

    //! 调用远程的接口
    template <typename T>
    int call(const std::string& name_, T& req_, FFRpcCallBack callback_ = NULL);

    //! call 接口的实现函数，call会将请求投递到该线程，保证所有请求有序
    int docall(const std::string& strServiceName_, const std::string& msg_name_, const std::string& body_, FFRpcCallBack callback_);
    //! 调用接口后，需要回调消息给请求者
    virtual void response(const std::string& msg_name_,  uint64_t destNodeId_,
                          int64_t callbackId_, const std::string& body_, std::string errinfo = "");
    //! 通过node id 发送消息给broker
    void sendToDestNode(const std::string& strServiceName_, const std::string& msg_name_,
                           uint64_t destNodeId_, int64_t callbackId_, const std::string& body_, std::string error_info = "");

    //! 定时重连 broker master
    void timerReconnectBroker();

    Timer& getTimer() { return m_timer; }

    //! 判断某个service是否存在
    bool isExist(const std::string& strServiceName_);

    //!获取broker socket
    SocketObjPtr getBrokerSocket();
    //! 新版 调用消息对应的回调函数
    int handleRpcCallMsg(SocketObjPtr sock_, BrokerRouteMsgReq& msg_);
    //! 注册接口【支持动态的增加接口】
    void reg(const std::string& name_, FFSlot::FFCallBack* func_);

    const std::string& getHostCfg(){ return m_host; }
    //!获取某一类型的service
    std::vector<std::string> getServicesLike(const std::string& token);
    std::string getServicesById(uint64_t destNodeId_);
    bool isOK() {return m_runing;}
private:
    SocketObjPtr connectToBroker(const std::string& host_, uint32_t nodeId_);

    //! 新版实现
    //! 处理注册,
    int handleBrokerRegResponse(SocketObjPtr sock_, RegisterToBrokerRet& msg_);

public:
    bool                                            m_runing;
private:
    std::string                                     m_host;
    std::string                                     m_strServiceName;//! 注册的服务名称
    uint64_t                                        m_nodeId;     //! 通过注册broker，分配的node id

    SharedPtr<TaskQueue>                            m_tq;
    Timer                                           m_timer;

    std::map<uint16_t, MsgCallBack>                 m_msgHandleFunc;//! 注册broker 消息的回调函数
    FFSlot                                          m_regInterface;//! 通过reg 注册的接口会暂时的存放在这里
    std::map<long, FFRpcCallBack>                   m_rpcTmpCallBack;//!
    SocketObjPtr                                    m_master_broker_sock;

    //!ff new impl
    RegisterToBrokerRet                             m_broker_data;
};

//! 注册接口
template <typename R, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (*func_)(RPCReq<IN_T, OUT_T>&))
{
    m_regInterface.bind(TYPE_NAME(IN_T), FFRpcOps::genCallBack(func_));
    return *this;
}
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&), CLASS_TYPE* obj)
{
    m_regInterface.bind(TYPE_NAME(IN_T), FFRpcOps::genCallBack(func_, obj));
    return *this;
}

struct FFRpc::SessionData
{
    SessionData(uint32_t n = 0):
        nodeId(n)
    {}
    uint32_t getNodeId() { return nodeId; }
    uint32_t nodeId;
};

//! 调用远程的接口
template <typename T>
int FFRpc::call(const std::string& name_, T& req_, FFRpcCallBack callback_)
{
    getTaskQueue().post(funcbind(&FFRpc::docall, this, name_, TYPE_NAME(T), FFThrift::EncodeAsString(req_), callback_));
    return 0;
}

}
#endif

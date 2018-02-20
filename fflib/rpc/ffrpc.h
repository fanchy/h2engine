//! 消息发送管理
#ifndef _FF_FFRPC_H_
#define _FF_FFRPC_H_

#include <string>
#include <map>
#include <vector>
#include <set>


#include "net/msg_handler.h"
#include "base/task_queue_impl.h"
#include "base/ffslot.h"
#include "net/codec.h"
#include "base/thread.h"
#include "rpc/ffrpc_ops.h"
#include "net/msg_sender.h"
#include "base/timer_service.h"
#include "base/arg_helper.h"

namespace ff {
class FFRpc: public MsgHandlerI, ffresponser_t
{
    struct session_data_t;
    struct slave_broker_info_t;
    struct broker_client_info_t;
public:
    FFRpc(std::string service_name_ = "");
    virtual ~FFRpc();

    int open(ArgHelper& arg_);
    int open(const std::string& broker_addr);
    int close();
    
    //! 处理连接断开
    int handleBroken(socket_ptr_t sock_);
    //! 处理消息
    int handleMsg(const Message& msg_, socket_ptr_t sock_);
    TaskQueueI* getTaskQueue();

    //! 注册接口
    template <typename R, typename IN_T, typename OUT_T>
    FFRpc& reg(R (*)(ffreq_t<IN_T, OUT_T>&));
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
    FFRpc& reg(R (CLASS_TYPE::*)(ffreq_t<IN_T, OUT_T>&), CLASS_TYPE* obj);

    //! 调用远程的接口
    template <typename T>
    int call(const std::string& name_, T& req_, FFSlot::FFCallBack* callback_ = NULL);
    //! 调用其他broker master 组的远程的接口
    template <typename T>
    int call(const std::string& namespace_, const std::string& name_, T& req_, FFSlot::FFCallBack* callback_ = NULL);
    
#ifdef FF_ENABLE_PROTOCOLBUF
    template <typename R, typename IN_T, typename OUT_T>
    FFRpc& reg(R (*)(ffreq_pb_t<IN_T, OUT_T>&));
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
    FFRpc& reg(R (CLASS_TYPE::*)(ffreq_pb_t<IN_T, OUT_T>&), CLASS_TYPE* obj);
    //! 调用远程的接口
    template <typename T>
    int callPB(const std::string& name_, T& req_, FFSlot::FFCallBack* callback_ = NULL);
    //! 调用其他broker master 组的远程的接口
    template <typename T>
    int callPB(const std::string& namespace_, const std::string& name_, T& req_, FFSlot::FFCallBack* callback_ = NULL);
#endif
    
    //! call 接口的实现函数，call会将请求投递到该线程，保证所有请求有序
    int call_impl(const std::string& service_name_, const std::string& msg_name_, const std::string& body_, FFSlot::FFCallBack* callback_);
    //! 调用接口后，需要回调消息给请求者
    virtual void response(const std::string& dest_namespace_, const std::string& msg_name_,  uint64_t dest_node_id_,
                          int64_t callback_id_, const std::string& body_, std::string err_info = "");
    //! 通过node id 发送消息给broker
    void sendToDestNode(const std::string& dest_namespace_, const std::string& service_name_, const std::string& msg_name_,
                           uint64_t dest_node_id_, int64_t callback_id_, const std::string& body_, std::string error_info = "");

    //! 定时重连 broker master
    void timer_reconnect_broker();

    TimerService& getTimer() { return m_timer; }
    //! 通过bridge broker调用远程的service
    int bridge_call_impl(const std::string& namespace_, const std::string& service_name_, const std::string& msg_name_,
                         const std::string& body_, FFSlot::FFCallBack* callback_);

    //! 判断某个service是否存在
    bool isExist(const std::string& service_name_);
    
    //!获取broker socket
    socket_ptr_t get_broker_socket();
    //! 新版 调用消息对应的回调函数
    int handle_rpc_call_msg(BrokerRouteMsg::in_t& msg_, socket_ptr_t sock_);
	//! 注册接口【支持动态的增加接口】
	void reg(const std::string& name_, FFSlot::FFCallBack* func_);
    
    const std::string& get_host(){ return m_host; }
    //!获取某一类型的service
    std::vector<std::string> getServicesLike(const std::string& token);
    std::string getServicesById(uint64_t dest_node_id_);
private:
    //! 处理连接断开
    //int handleBroken_impl(socket_ptr_t sock_);
    //! 处理消息
    //int handleMsg_impl(const Message& msg_, socket_ptr_t sock_);
    //! 连接到broker master
    socket_ptr_t connect_to_broker(const std::string& host_, uint32_t node_id_);
    
    //! 新版实现
    //! 处理注册, 
    int handle_broker_reg_response(RegisterToBroker::out_t& msg_, socket_ptr_t sock_);
    
public:
    bool                                            m_runing;
private:
    std::string                                     m_host;
    std::string                                     m_service_name;//! 注册的服务名称
    uint64_t                                        m_node_id;     //! 通过注册broker，分配的node id
    
    TaskQueue                                       m_tq;
    TimerService                                    m_timer;
    Thread                                          m_thread;
    FFSlot                                          m_ffslot;//! 注册broker 消息的回调函数
    FFSlot                                          m_ffslot_interface;//! 通过reg 注册的接口会暂时的存放在这里
    FFSlot                                          m_ffslot_callback;//! 
    socket_ptr_t                                    m_master_broker_sock;

    std::map<uint32_t, slave_broker_info_t>         m_slave_broker_sockets;//! node id -> info
    std::map<std::string, uint32_t>                 m_msg2id;
    std::map<uint32_t, broker_client_info_t>        m_broker_client_info;//! node id -> service
    std::map<std::string, uint32_t>                 m_broker_client_name2nodeid;//! service name -> service node id
    
    //!ff绑定的broker id
    uint64_t                                        m_bind_broker_id;
    //!所有的broker socket
    std::map<uint64_t, socket_ptr_t>                m_broker_sockets;
    //!ff new impl
    RegisterToBroker::out_t                         m_broker_data;
};

//! 注册接口
template <typename R, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (*func_)(ffreq_t<IN_T, OUT_T>&))
{
    m_ffslot_interface.bind(TYPE_NAME(IN_T), FFRpcOps::gen_callback(func_));
    return *this;
}
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (CLASS_TYPE::*func_)(ffreq_t<IN_T, OUT_T>&), CLASS_TYPE* obj)
{
    m_ffslot_interface.bind(TYPE_NAME(IN_T), FFRpcOps::gen_callback(func_, obj));
    return *this;
}

struct FFRpc::session_data_t
{
    session_data_t(uint32_t n = 0):
        node_id(n)
    {}
    uint32_t get_node_id() { return node_id; }
    uint32_t node_id;
};
struct FFRpc::slave_broker_info_t
{
    slave_broker_info_t():
        sock(NULL)
    {}
    std::string          host;
    socket_ptr_t    sock;
};
struct FFRpc::broker_client_info_t
{
    broker_client_info_t():
        bind_broker_id(0),
        service_id(0)
    {}
    uint32_t bind_broker_id;
    std::string   service_name;
    uint16_t service_id;
};

//! 调用远程的接口
template <typename T>
int FFRpc::call(const std::string& name_, T& req_, FFSlot::FFCallBack* callback_)
{
    m_tq.post(TaskBinder::gen(&FFRpc::call_impl, this, name_, TYPE_NAME(T), FFThrift::EncodeAsString(req_), callback_));
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
    else{
        m_tq.post(TaskBinder::gen(&FFRpc::bridge_call_impl, this, namespace_, name_, TYPE_NAME(T), FFThrift::EncodeAsString(req_), callback_));
    }
    return 0;
}

#ifdef FF_ENABLE_PROTOCOLBUF
template <typename R, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (*func_)(ffreq_pb_t<IN_T, OUT_T>&))
{
    m_ffslot_interface.bind(TYPE_NAME(IN_T), FFRpcOps::gen_callback(func_));
    return *this;
}
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
FFRpc& FFRpc::reg(R (CLASS_TYPE::*func_)(ffreq_pb_t<IN_T, OUT_T>&), CLASS_TYPE* obj)
{
    m_ffslot_interface.bind(TYPE_NAME(IN_T), FFRpcOps::gen_callback(func_, obj));
    return *this;
}


//! 调用远程的接口
template <typename T>
int FFRpc::callPB(const std::string& name_, T& req_, FFSlot::FFCallBack* callback_)
{
    std::string ret;
    req_.SerializeToString(&ret);
    m_tq.post(TaskBinder::gen(&FFRpc::call_impl, this, name_, TYPE_NAME(T), ret, callback_));
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
        m_tq.post(TaskBinder::gen(&FFRpc::bridge_call_impl, this, namespace_, name_, TYPE_NAME(T), ret, callback_));
    }
    return 0;
}
#endif
}
#endif

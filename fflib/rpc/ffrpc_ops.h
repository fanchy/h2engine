
#ifndef _FF_RPC_OPS_H_
#define _FF_RPC_OPS_H_

#include <assert.h>
#include <string>


#include "base/ffslot.h"
#include "net/socket.h"
#include "base/fftype.h"
#include "net/codec.h"
#include "base/singleton.h"

#ifdef FF_ENABLE_PROTOCOLBUF //! 如果需要开启对protobuf的支持，需要开启这个宏
#include <google/protobuf/message.h>
#endif

#include <thrift/FFThrift.h>

#include "rpc/msg_def/ffrpc_msg_types.h"
#include "rpc/msg_def/ffrpc_msg_constants.h"
using namespace ffrpc_msg;

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
enum node_type_e
{
    BRIDGE_BROKER, //! 连接各个区服的代理服务器
    MASTER_BROKER, //! 每个区服的主服务器
    SLAVE_BROKER,  //! 从服务器
    RPC_NODE,      //! rpc节点
    SYNC_CLIENT_NODE, //! 同步调用client
};

#define BROKER_MASTER_NODE_ID   0
#define GEN_SERVICE_NAME(M, X, Y) snprintf(M, sizeof(M), "%s@%u", X, Y)
#define RECONNECT_TO_BROKER_TIMEOUT       1000//! ms
#define RECONNECT_TO_BROKER_BRIDGE_TIMEOUT       1000//! ms
    
class SlotMsgArg: public FFSlot::CallBackArg
{
public:
    SlotMsgArg(const std::string& s_, SocketPtr sock_):
        body(s_),
        sock(sock_)
    {}
    virtual int type()
    {
        return TYPEID(SlotMsgArg);
    }
    std::string       body;
    SocketPtr sock;
};

struct MsgTool
{
    static std::string encode(msg_i& msg_)
    {
        return msg_.encode_data();
    }
    template<typename T>
    static int decode(T& msg_, const std::string& data_, std::string* err_ = NULL)
    {
        try{
            //msg_.decode_data(data_);
            FFThrift::DecodeFromString(msg_, data_);
        }
        catch(std::exception& e)
        {
            if (err_) *err_ += e.what();
        }
        return 0;
    }

};

class RPCResponser
{
public:
    virtual ~RPCResponser(){}
    virtual void response(const std::string& dest_namespace_, const std::string& msg_name_,  uint64_t dest_node_id_,
                          int64_t callback_id_, const std::string& body_, std::string err = "") = 0;
};

class SlotReqArg: public FFSlot::CallBackArg
{
public:
    SlotReqArg(const std::string& s_, uint64_t n_, int64_t cb_id_, const std::string& name_space_, std::string err_info_, RPCResponser* p):
        body(s_),
        dest_node_id(n_),
        callback_id(cb_id_),
        dest_namespace(name_space_),
        err_info(err_info_),
        responser(p)
    {}
    SlotReqArg(){}
    SlotReqArg(const SlotReqArg& src):
        body(src.body),
        dest_node_id(src.dest_node_id),
        callback_id(src.callback_id),
        dest_namespace(src.dest_namespace),
        err_info(src.err_info),
        responser(src.responser)
    {}
    SlotReqArg& operator=(const SlotReqArg& src)
    {
        body = src.body,
        dest_node_id = src.dest_node_id;
        callback_id = src.callback_id;
        dest_namespace = src.dest_namespace;
        err_info = src.err_info;
        responser = src.responser;
        return *this;
    }
    virtual int type()
    {
        return TYPEID(SlotReqArg);
    }
    std::string          body;
    uint64_t             dest_node_id;//! 请求来自于那个node id
    int64_t              callback_id;//! 回调函数标识id
    std::string          dest_namespace;
    std::string          err_info;
    RPCResponser*        responser;
};



class null_type_t: public FFMsg<null_type_t>
{
    void encode(){}
    void decode(){}
};

template<typename IN_T, typename OUT_T = null_type_t>
struct RPCReq
{
    RPCReq():
        dest_node_id(0),
        callback_id(0),
        responser(NULL)
    {}
    bool error() const { return err_info.empty() == false; }
    const std::string& errorMsg() const { return err_info; }
    
    void response(OUT_T& out_)
    {
        if (0 != callback_id)
        {
        	responser->response(dest_namespace, TYPE_NAME(OUT_T), dest_node_id, callback_id, FFThrift::EncodeAsString(out_));
            callback_id = 0;
		}
            
    }
    IN_T                 msg;
    std::string          dest_namespace;
    uint64_t             dest_node_id;
    int64_t              callback_id;
    RPCResponser*        responser;
    std::string          err_info;
};

#ifdef FF_ENABLE_PROTOCOLBUF

template<typename IN_T, typename OUT = null_type_t>
struct RPCReqPB
{
    RPCReqPB():
        dest_node_id(0),
        callback_id(0),
        responser(NULL)
    {}
    bool error() const { return err_info.empty() == false; }
    const std::string& errorMsg() const { return err_info; }
    IN              msg;

    std::string          dest_namespace;
    uint64_t        dest_node_id;
    int64_t        callback_id;
    RPCResponser*  responser;
    std::string          err_info;
    void response(OUT& out_)
    {
        if (0 != callback_id){
            std::string ret;
            out_.SerializeToString(&ret);
            responser->response(dest_namespace, TYPE_NAME(OUT), dest_node_id, callback_id, ret);
        }
    }
};

#endif

struct FFRpcOps
{
    //! 接受broker 消息的回调函数
    template <typename R, typename T>
    static FFSlot::FFCallBack* genCallBack(R (*)(T&, SocketPtr));
    template <typename R, typename CLASS_TYPE, typename T>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*)(T&, SocketPtr), CLASS_TYPE* obj_);
    
    //! broker client 注册接口
    template <typename R, typename IN_T, typename OUT_T>
    static FFSlot::FFCallBack* genCallBack(R (*)(RPCReq<IN_T, OUT_T>&));
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*)(RPCReq<IN_T, OUT_T>&), CLASS_TYPE* obj);

    //! ffrpc call接口的回调函数参数
    //! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1), CLASS_TYPE* obj_, ARG1 arg1_);
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1,
              typename FUNC_ARG2, typename ARG2>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_);
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1,
              typename FUNC_ARG2, typename ARG2, typename FUNC_ARG3, typename ARG3>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_);
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1,
              typename FUNC_ARG2, typename ARG2, typename FUNC_ARG3, typename ARG3, typename FUNC_ARG4, typename ARG4>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3, FUNC_ARG4),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_);
    
    //!******************************** pb 使用不同的callback函数 ******************************************

#ifdef FF_ENABLE_PROTOCOLBUF

    template <typename R, typename IN_T, typename OUT_T>
    static FFSlot::FFCallBack* genCallBack(R (*)(RPCReqPB<IN_T, OUT_T>&));
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*)(RPCReqPB<IN_T, OUT_T>&), CLASS_TYPE* obj);

    //! ffrpc call接口的回调函数参数
    //! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1), CLASS_TYPE* obj_, ARG1 arg1_);
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1,
              typename FUNC_ARG2, typename ARG2>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_);
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1,
              typename FUNC_ARG2, typename ARG2, typename FUNC_ARG3, typename ARG3>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_);
    template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1,
              typename FUNC_ARG2, typename ARG2, typename FUNC_ARG3, typename ARG3, typename FUNC_ARG4, typename ARG4>
    static FFSlot::FFCallBack* genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3, FUNC_ARG4),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_);
    
#endif
};

template <typename R, typename T>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (*func_)(T&, SocketPtr))
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (*func_t)(T&, SocketPtr);
        lambda_cb(func_t func_):m_func(func_){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotMsgArg))
            {
                return;
            }
            SlotMsgArg* msg_data = (SlotMsgArg*)args_;
            T msg;
            MsgTool::decode(msg, msg_data->body);
            m_func(msg, msg_data->sock);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func); }
        func_t m_func;
    };
    return new lambda_cb(func_);
}
template <typename R, typename CLASS_TYPE, typename T>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(T&, SocketPtr), CLASS_TYPE* obj_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(T&, SocketPtr);
        lambda_cb(func_t func_, CLASS_TYPE* obj_):m_func(func_), m_obj(obj_){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotMsgArg))
            {
                return;
            }
            SlotMsgArg* msg_data = (SlotMsgArg*)args_;
            T msg;
            MsgTool::decode(msg, msg_data->body);
            (m_obj->*(m_func))(msg, msg_data->sock);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
    };
    return new lambda_cb(func_, obj_);
}

//! 注册接口
template <typename R, typename IN_T, typename OUT_T>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (*func_)(RPCReq<IN_T, OUT_T>&))
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (*func_t)(RPCReq<IN_T, OUT_T>&);
        lambda_cb(func_t func_):m_func(func_){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReq<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                MsgTool::decode(req.msg, msg_data->body, &(req.err_info));
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            m_func(req);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func); }
        func_t m_func;
    };
    return new lambda_cb(func_);
}
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&), CLASS_TYPE* obj_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReq<IN_T, OUT_T>&);
        lambda_cb(func_t func_, CLASS_TYPE* obj_):m_func(func_), m_obj(obj_){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReq<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                MsgTool::decode(req.msg, msg_data->body, &(req.err_info));
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
    };
    return new lambda_cb(func_, obj_);
}

//! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1), CLASS_TYPE* obj_, ARG1 arg1_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1);
        lambda_cb(func_t func_, CLASS_TYPE* obj_, const ARG1& arg1_):
            m_func(func_), m_obj(obj_), m_arg1(arg1_){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReq<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                MsgTool::decode(req.msg, msg_data->body, &(req.err_info));
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req, m_arg1);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj, m_arg1); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
        typename RefTypeTraits<ARG1>::RealType m_arg1;
    };
    return new lambda_cb(func_, obj_, arg1_);
}

//! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1, typename FUNC_ARG2, typename ARG2>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2);
        lambda_cb(func_t func_, CLASS_TYPE* obj_, const ARG1& arg1_, const ARG2& arg2_):
            m_func(func_), m_obj(obj_), m_arg1(arg1_), m_arg2(arg2_)
        {}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReq<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                MsgTool::decode(req.msg, msg_data->body, &(req.err_info));
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req, m_arg1, m_arg2);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj, m_arg1, m_arg2); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
        typename RefTypeTraits<ARG1>::RealType m_arg1;
        typename RefTypeTraits<ARG2>::RealType m_arg2;
    };
    return new lambda_cb(func_, obj_, arg1_, arg2_);
}

//! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1, typename FUNC_ARG2, typename ARG2,
          typename FUNC_ARG3, typename ARG3>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3);
        lambda_cb(func_t func_, CLASS_TYPE* obj_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_):
            m_func(func_), m_obj(obj_), m_arg1(arg1_), m_arg2(arg2_), m_arg3(arg3_)
        {}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReq<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                MsgTool::decode(req.msg, msg_data->body, &(req.err_info));
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req, m_arg1, m_arg2, m_arg3);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj, m_arg1, m_arg2, m_arg3); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
        typename RefTypeTraits<ARG1>::RealType m_arg1;
        typename RefTypeTraits<ARG2>::RealType m_arg2;
        typename RefTypeTraits<ARG3>::RealType m_arg3;
    };
    return new lambda_cb(func_, obj_, arg1_, arg2_, arg3_);
}

//! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1, typename FUNC_ARG2, typename ARG2,
          typename FUNC_ARG3, typename ARG3, typename FUNC_ARG4, typename ARG4>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3, FUNC_ARG4),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReq<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3, FUNC_ARG4);
        lambda_cb(func_t func_, CLASS_TYPE* obj_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_):
            m_func(func_), m_obj(obj_), m_arg1(arg1_), m_arg2(arg2_), m_arg3(arg3_), m_arg4(arg4_)
        {}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReq<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                MsgTool::decode(req.msg, msg_data->body);
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req, m_arg1, m_arg2, m_arg3, m_arg4);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj, m_arg1, m_arg2, m_arg3, m_arg4); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
        typename RefTypeTraits<ARG1>::RealType m_arg1;
        typename RefTypeTraits<ARG2>::RealType m_arg2;
        typename RefTypeTraits<ARG3>::RealType m_arg3;
        typename RefTypeTraits<ARG4>::RealType m_arg4;
    };
    return new lambda_cb(func_, obj_, arg1_, arg2_, arg3_, arg4_);
}

//!******************************** pb 使用不同的callback函数 ******************************************

#ifdef FF_ENABLE_PROTOCOLBUF

template <typename R, typename IN_T, typename OUT_T>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (*func_)(RPCReqPB<IN_T, OUT_T>&))
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (*func_t)(RPCReqPB<IN_T, OUT_T>&);
        lambda_cb(func_t func_):m_func(func_){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReqPB<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                req.msg.ParseFromString(msg_data->body);
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            m_func(req);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func); }
        func_t m_func;
    };
    return new lambda_cb(func_);
}
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&), CLASS_TYPE* obj_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReqPB<IN_T, OUT_T>&);
        lambda_cb(func_t func_, CLASS_TYPE* obj_):m_func(func_), m_obj(obj_){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReqPB<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                req.msg.ParseFromString(msg_data->body);
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
    };
    return new lambda_cb(func_, obj_);
}

//! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1), CLASS_TYPE* obj_, ARG1 arg1_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1);
        lambda_cb(func_t func_, CLASS_TYPE* obj_, const ARG1& arg1_):
            m_func(func_), m_obj(obj_), m_arg1(arg1_){}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReqPB<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                req.msg.ParseFromString(msg_data->body);
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req, m_arg1);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj, m_arg1); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
        typename RefTypeTraits<ARG1>::RealType m_arg1;
    };
    return new lambda_cb(func_, obj_, arg1_);
}

//! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1, typename FUNC_ARG2, typename ARG2>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2);
        lambda_cb(func_t func_, CLASS_TYPE* obj_, const ARG1& arg1_, const ARG2& arg2_):
            m_func(func_), m_obj(obj_), m_arg1(arg1_), m_arg2(arg2_)
        {}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReqPB<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                req.msg.ParseFromString(msg_data->body);
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req, m_arg1, m_arg2);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj, m_arg1, m_arg2); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
        typename RefTypeTraits<ARG1>::RealType m_arg1;
        typename RefTypeTraits<ARG2>::RealType m_arg2;
    };
    return new lambda_cb(func_, obj_, arg1_, arg2_);
}

//! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1, typename FUNC_ARG2, typename ARG2,
          typename FUNC_ARG3, typename ARG3>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3);
        lambda_cb(func_t func_, CLASS_TYPE* obj_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_):
            m_func(func_), m_obj(obj_), m_arg1(arg1_), m_arg2(arg2_), m_arg3(arg3_)
        {}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReqPB<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                req.msg.ParseFromString(msg_data->body);
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req, m_arg1, m_arg2, m_arg3);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj, m_arg1, m_arg2, m_arg3); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
        typename RefTypeTraits<ARG1>::RealType m_arg1;
        typename RefTypeTraits<ARG2>::RealType m_arg2;
        typename RefTypeTraits<ARG3>::RealType m_arg3;
    };
    return new lambda_cb(func_, obj_, arg1_, arg2_, arg3_);
}

//! 如果绑定回调函数的时候，有时需要一些临时参数被保存直到回调函数被调用
template <typename R, typename CLASS_TYPE, typename IN_T, typename OUT_T, typename FUNC_ARG1, typename ARG1, typename FUNC_ARG2, typename ARG2,
          typename FUNC_ARG3, typename ARG3, typename FUNC_ARG4, typename ARG4>
FFSlot::FFCallBack* FFRpcOps::genCallBack(R (CLASS_TYPE::*func_)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3, FUNC_ARG4),
                                                CLASS_TYPE* obj_, ARG1 arg1_, ARG2 arg2_, ARG3 arg3_, ARG4 arg4_)
{
    struct lambda_cb: public FFSlot::FFCallBack
    {
        typedef R (CLASS_TYPE::*func_t)(RPCReqPB<IN_T, OUT_T>&, FUNC_ARG1, FUNC_ARG2, FUNC_ARG3, FUNC_ARG4);
        lambda_cb(func_t func_, CLASS_TYPE* obj_, const ARG1& arg1_, const ARG2& arg2_, const ARG3& arg3_, const ARG4& arg4_):
            m_func(func_), m_obj(obj_), m_arg1(arg1_), m_arg2(arg2_), m_arg3(arg3_), m_arg4(arg4_)
        {}
        virtual void exe(FFSlot::CallBackArg* args_)
        {
            if (args_->type() != TYPEID(SlotReqArg))
            {
                return;
            }
            SlotReqArg* msg_data = (SlotReqArg*)args_;
            RPCReqPB<IN_T, OUT_T> req;
            if (msg_data->err_info.empty())
            {
                req.msg.ParseFromString(msg_data->body);
            }
            else
            {
                req.err_info = msg_data->err_info;
            }
            req.dest_node_id = msg_data->dest_node_id;
            req.callback_id = msg_data->callback_id;
            req.dest_namespace = msg_data->dest_namespace;
            req.responser = msg_data->responser;
            (m_obj->*(m_func))(req, m_arg1, m_arg2, m_arg3, m_arg4);
        }
        virtual FFSlot::FFCallBack* fork() { return new lambda_cb(m_func, m_obj, m_arg1, m_arg2, m_arg3, m_arg4); }
        func_t      m_func;
        CLASS_TYPE* m_obj;
        typename RefTypeTraits<ARG1>::RealType m_arg1;
        typename RefTypeTraits<ARG2>::RealType m_arg2;
        typename RefTypeTraits<ARG3>::RealType m_arg3;
        typename RefTypeTraits<ARG4>::RealType m_arg4;
    };
    return new lambda_cb(func_, obj_, arg1_, arg2_, arg3_, arg4_);
}
#endif

enum ffrpc_cmd_def_e
{
    BROKER_TO_CLIENT_MSG = 1,
    //!新版本*********
    REGISTER_TO_BROKER_REQ,
    REGISTER_TO_BROKER_RET,
    BROKER_ROUTE_MSG,
    SYNC_CLIENT_REQ, //! 同步客户端的请求，如python,php
};


//! gate 验证client的sessionid的消息
// struct session_verify_t
// {
    // typedef session_verify_in_t  in_t;
    // typedef session_verify_out_t out_t;
// };

//!session 第一次进入
// struct session_first_entere_t
// {
    // typedef session_first_entere_in_t  in_t;
    // typedef session_first_entere_out_t out_t;
// };
//! gate session 进入场景服务器消息
struct SessionEnterWorker
{
    typedef session_enter_worker_in_t  in_t;
    typedef session_enter_worker_out_t out_t;
    
};
//! gate session 下线
struct SessionOffline
{
    typedef session_offline_in_t  in_t;
    typedef session_offline_out_t out_t;
    
};

//! gate 转发client的消息
struct RouteLogicMsg_t
{
    typedef routeLogicMsg_in_t  in_t;
    typedef routeLogicMsg_out_t out_t;
};
//! 改变gate 中client 对应的logic节点
struct GateChangeLogicNode
{
    typedef gate_change_logic_node_in_t  in_t;
    typedef gate_change_logic_node_out_t out_t;
    
};

//! 关闭gate中的某个session
struct GateCloseSession
{
    typedef gate_closeSession_in_t  in_t;
    typedef gate_closeSession_out_t out_t;
    
};
//! 转发消息给client
struct GateRouteMsgToSession
{
    typedef gate_routeMmsgToSession_in_t  in_t;
    typedef gate_routeMmsgToSession_out_t out_t;
    
};
//! 转发消息给所有client
struct GateBroadcastMsgToSession
{
    typedef gate_broadcastMsgToSession_in_t  in_t;
    typedef gate_broadcastMsgToSession_out_t out_t;
};
//! login 日志
// struct user_login_event_t
// {
    // typedef user_login_event_in_t  in_t;
    // typedef empty_ret_msg out_t;
// };
//! login 日志
// struct user_logout_event_t
// {
    // typedef user_logout_event_in_t  in_t;
    // typedef empty_ret_msg out_t;
// };
//! scene 之间的互调用

struct WorkerCallMsgt
{
    typedef worker_call_msg_in_t  in_t;
    typedef worker_call_msg_out_t out_t;
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
    int addNode(uint64_t node_id_, FFRpc* ffrpc_)
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
        tmp_data[node_id_].ffrpc = ffrpc_;
        m_node_info.update_data(tmp_data);
        return 0;
    }
    int addNode(uint64_t node_id_, FFBroker* ffbroker_)
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
        tmp_data[node_id_].ffbroker = ffbroker_;
        m_node_info.update_data(tmp_data);
        return 0;
    }
    int delNode(uint64_t node_id_)
    {
        LockGuard lock(m_mutex);
        node_info_map_t tmp_data = m_node_info.get_data();
        tmp_data.erase(node_id_);
        m_node_info.update_data(tmp_data);
        return 0;
    }
    FFRpc* getRpc(uint64_t node_id_)
    {
        const node_info_map_t& tmp_data = m_node_info.get_data();
        node_info_map_t::const_iterator it = tmp_data.find(node_id_);
        if (it != tmp_data.end())
        {
            return it->second.ffrpc;
        }
        return NULL;
    }
    FFBroker* getBroker(uint64_t node_id_)
    {
        const node_info_map_t& tmp_data = m_node_info.get_data();
        node_info_map_t::const_iterator it = tmp_data.find(node_id_);
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

//!新版本的实现*********************************************************************************************

//! 处理其他broker或者client注册到此server
struct RegisterToBroker
{
    typedef register_to_broker_in_t  in_t;
    typedef register_to_broker_out_t out_t;
};
//! 处理转发消息的操作
struct BrokerRouteMsg
{
    typedef broker_route_msg_in_t in_t;
};


}

#endif

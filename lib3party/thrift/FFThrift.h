#ifndef _THRIFT_FFTHRIFT_H_
#define _THRIFT_FFTHRIFT_H_ 1

#include <string>

#include <thrift/Thrift.h>
#include <thrift/transport/TTransport.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/FFTransport.h>

#ifdef TYPE_NAME
#define GET_TYPE_NAME_FFTHRIFT(X) TYPE_NAME(X)
#else
#define GET_TYPE_NAME_FFTHRIFT(X) "ff"
#endif

//#include <stdio.h>

struct FFThrift
{
    template<typename T>
    static bool EncodeToString(T& msg_, std::string& data_)
    {
        using apache::thrift::transport::FFTMemoryBuffer;
        using apache::thrift::protocol::TBinaryProtocol;
        FFTMemoryBuffer ffmem(&data_);
        TBinaryProtocol  proto(&ffmem);
        proto.writeMessageBegin(GET_TYPE_NAME_FFTHRIFT(T), (apache::thrift::protocol::TMessageType)0, (int32_t)0);
        msg_.write(&proto);
        proto.writeMessageEnd();
        return true;
    }
    template<typename T>
    static std::string EncodeAsString(T& msg_)
    {
        std::string ret;
        FFThrift::EncodeToString(msg_, ret);
        return ret;
    }
    template<typename T>
    static bool DecodeFromString(T& msg_, const std::string& data_)
    {
        using apache::thrift::transport::FFTMemoryBuffer;
        using apache::thrift::protocol::TBinaryProtocol;
        FFTMemoryBuffer ffmem(data_);
        TBinaryProtocol  proto(&ffmem);
        std::string msg_name;
        apache::thrift::protocol::TMessageType nType = (apache::thrift::protocol::TMessageType)0;
        int32_t nFlag = 0;
        proto.readMessageBegin(msg_name, nType, nFlag);
        msg_.read(&proto);
        proto.readMessageEnd();
        //printf("msg=%s\n", msg_name.c_str());
        return true;   
    }
};

#endif

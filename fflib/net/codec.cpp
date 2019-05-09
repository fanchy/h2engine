
#include "net/codec.h"

#include <thrift/FFThrift.h>
using namespace std;
using namespace ff;
void FFThriftCodecTool::write(msg_i& msg, ::apache::thrift::protocol::TProtocol* iprot)
{
    string tmp = msg.encode_data();
    ::apache::thrift::protocol::TTransport* trans_ = iprot->getTransport();
    trans_->write((const uint8_t*)tmp.c_str(), tmp.size());
}
void FFThriftCodecTool::read(msg_i& msg, ::apache::thrift::protocol::TProtocol* iprot)
{
    ::apache::thrift::protocol::TTransport* trans_ = iprot->getTransport();
    apache::thrift::transport::FFTMemoryBuffer* pmem = (apache::thrift::transport::FFTMemoryBuffer*)trans_;
    msg.decode_data(pmem->get_rbuff());
}


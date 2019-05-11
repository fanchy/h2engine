//! 二进制序列化 
#ifndef _CODEC_H_
#define _CODEC_H_

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#include <string>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <map>
#include <set>
#ifdef linux
#include <endian.h>
#include <byteswap.h>
#endif


#include "net/message.h"
#include "base/singleton.h"
#include "base/atomic_op.h"
#include "base/lock.h"
#include "base/fftype.h"
#include "base/smart_ptr.h"

namespace apache
{
    namespace thrift{
        namespace protocol
        {
            class TProtocol;
        }
    }
}
namespace ff {
#ifndef linux

#define __bswap_64(val) (((val) >> 56) |\
                    (((val) & 0x00ff000000000000ll) >> 40) |\
                    (((val) & 0x0000ff0000000000ll) >> 24) |\
                    (((val) & 0x000000ff00000000ll) >> 8)   |\
                    (((val) & 0x00000000ff000000ll) << 8)   |\
                    (((val) & 0x0000000000ff0000ll) << 24) |\
                    (((val) & 0x000000000000ff00ll) << 40) |\
                    (((val) << 56)))

#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
    #define hton64(ll) (__bswap_64(ll))
    #define ntoh64(ll) (__bswap_64(ll))
#else
    #define hton64(ll) (ll)
    #define ntoh64(ll) (ll)
#endif

struct Codec
{
    virtual ~Codec(){}
    virtual std::string encode_data()                      = 0;
    virtual void decode_data(const std::string& src_buff_) = 0;
};

class BinEncoder;
class BinDecoder;
struct CodecHelper
{
    virtual ~CodecHelper(){}
    virtual void encode(BinEncoder&) const = 0;
    virtual void decode(BinDecoder&)       = 0;
};


template<typename T>
struct CodecTool;
class BinEncoder
{
public:
    const std::string& get_buff() const { return m_dest_buff; }
    
    template<typename T>
    BinEncoder& operator << (const T& val_);

    void clear()
    {
        m_dest_buff.clear();
    }
    inline BinEncoder& copy_value(const void* src_, size_t size_)
    {
        m_dest_buff.append((const char*)(src_), size_);
        return *this;
    }
    inline BinEncoder& copy_value(const std::string& str_)
    {
        uint32_t str_size =htonl( str_.size());
        copy_value((const char*)(&str_size), sizeof(str_size));
        copy_value(str_.data(), str_.size());
        return *this;
    }

private:
    std::string         m_dest_buff;
};

class BinDecoder
{
public:
    BinDecoder():
        m_index_ptr(NULL),
        m_remaindered(0)
    {}
    explicit BinDecoder(const std::string& src_buff_):
        m_index_ptr(src_buff_.data()),
        m_remaindered(src_buff_.size())
    {
    }
    BinDecoder& init(const std::string& str_buff_)
    {
        m_index_ptr = str_buff_.data();
        m_remaindered = str_buff_.size();
        return *this;
    }
    template<typename T>
    BinDecoder& operator >> (T& val_);

    BinDecoder& copy_value(void* dest_, uint32_t var_size_)
    {
        if (m_remaindered < var_size_)
        {
            throw std::runtime_error("BinDecoder:msg size not enough");
        }
        ::memcpy(dest_, m_index_ptr, var_size_);
        m_index_ptr     += var_size_;
        m_remaindered  -= var_size_;
        return *this;
    }
    
    BinDecoder& copy_value(std::string& dest_)
    {
        uint32_t str_len = 0;
        copy_value(&str_len, sizeof(str_len));
        str_len = ntohl(str_len);

        if (m_remaindered < str_len)
        {
            throw std::runtime_error("BinDecoder:msg size not enough");
        }
        
        dest_.assign((const char*)m_index_ptr, str_len);
        m_index_ptr     += str_len;
        m_remaindered   -= str_len;
        return *this;
    }

private:
    const char*  m_index_ptr;
    size_t       m_remaindered;
};


//! 用于traits 序列化的参数
template<typename T>
struct CodecTool
{
    static void encode(BinEncoder& en_, const T& val_)
    {
        T* pobj = (T*)(&val_);
        en_ << pobj->encode_data();
    }
    static void decode(BinDecoder& de_, T& val_)
    {
        std::string data;
        de_ >> data;
        val_.decode_data(data);
    }
};

//! 用于traits 序列化的参数
#define GEN_CODE_ENCODE_DECODE(X)                                          \
template<>                                                          \
struct CodecTool<X>                                              \
{                                                                   \
    static void encode(BinEncoder& en_, const X& val_)           \
    {                                                               \
        en_.copy_value((const char*)(&val_), sizeof(val_));         \
    }                                                               \
    static void decode(BinDecoder& de_, X& val_)           \
    {                                                               \
        de_.copy_value((void*)(&val_), sizeof(val_));         \
    }                                                               \
};

#define GEN_CODE_ENCODE_DECODE_SHORT(X)                                          \
template<>                                                          \
struct CodecTool<X>                                              \
{                                                                   \
    static void encode(BinEncoder& en_, const X& src_val_)       \
    {                                                               \
        X val_ = (X)htons(src_val_);                                 \
        en_.copy_value((const char*)(&val_), sizeof(val_));         \
    }                                                               \
    static void decode(BinDecoder& de_, X& val_)                 \
    {                                                               \
        de_.copy_value((void*)(&val_), sizeof(val_));               \
        val_ = (X)ntohs(val_);                                       \
    }                                                               \
};
#define GEN_CODE_ENCODE_DECODE_LONG(X)                                          \
template<>                                                          \
struct CodecTool<X>                                              \
{                                                                   \
    static void encode(BinEncoder& en_, const X& src_val_)       \
    {                                                               \
        X val_ = (X)htonl(src_val_);                                 \
        en_.copy_value((const char*)(&val_), sizeof(val_));         \
    }                                                               \
    static void decode(BinDecoder& de_, X& val_)                 \
    {                                                               \
        de_.copy_value((void*)(&val_), sizeof(val_));               \
        val_ = (X)ntohl(val_);                                       \
    }                                                               \
};
#define GEN_CODE_ENCODE_DECODE_64(X)                                \
template<>                                                          \
struct CodecTool<X>                                              \
{                                                                   \
    static void encode(BinEncoder& en_, const X& src_val_)       \
    {                                                               \
        X val_ = (X)(hton64(src_val_));                                 \
        en_.copy_value((const char*)(&val_), sizeof(val_));         \
    }                                                               \
    static void decode(BinDecoder& de_, X& val_)                 \
    {                                                               \
        de_.copy_value((void*)(&val_), sizeof(val_));               \
        val_ = (X)(ntoh64(val_));                                     \
    }                                                               \
};


//! 用于traits 序列化的参数
template<>
struct CodecTool<std::string>
{
    static void encode(BinEncoder& en_, const std::string& val_)
    {
        en_.copy_value(val_);
    }
    static void decode(BinDecoder& de_, std::string& val_)
    {
        de_.copy_value(val_);
    }
};
template<typename T>
struct CodecTool<std::vector<T> >
{
    static void encode(BinEncoder& en_, const std::vector<T>& val_)
    {
        uint32_t num = val_.size();
        en_.copy_value((const char*)(&num), sizeof(num));
        typename std::vector<T>::const_iterator it = val_.begin();
        for (; it != val_.end(); ++it)
        {
            en_ << (*it);
        }
    }
    static void decode(BinDecoder& de_, std::vector<T>& val_)
    {
        uint32_t num = 0;
        de_.copy_value((void*)(&num), sizeof(num));
        for (uint32_t i = 0; i < num; ++i)
        {
            T tmp_key;
            de_ >> tmp_key;
            val_.push_back(tmp_key);
        }
    }
};
template<typename T>
struct CodecTool<std::list<T> >
{
    static void encode(BinEncoder& en_, const std::list<T>& val_)
    {
        uint32_t num = val_.size();
        en_.copy_value((const char*)(&num), sizeof(num));
        typename std::list<T>::const_iterator it = val_.begin();
        for (; it != val_.end(); ++it)
        {
            en_ << (*it);
        }
    }
    static void decode(BinDecoder& de_, std::list<T>& val_)
    {
        uint32_t num = 0;
        de_.copy_value((void*)(&num), sizeof(num));
        for (uint32_t i = 0; i < num; ++i)
        {
            T tmp_key;
            de_ >> tmp_key;
            val_.push_back(tmp_key);
        }
    }
};
template<typename T>
struct CodecTool<std::set<T> >
{
    static void encode(BinEncoder& en_, const std::set<T>& val_)
    {
        uint32_t num = val_.size();
        en_.copy_value((const char*)(&num), sizeof(num));
        typename std::set<T>::const_iterator it = val_.begin();
        for (; it != val_.end(); ++it)
        {
            en_ << (*it);
        }
    }
    static void decode(BinDecoder& de_, std::set<T>& val_)
    {
        uint32_t num = 0;
        de_.copy_value((void*)(&num), sizeof(num));
        for (uint32_t i = 0; i < num; ++i)
        {
            T tmp_key;
            de_ >> tmp_key;
            val_.insert(tmp_key);
        }
    }
};
template<typename T, typename R>
struct CodecTool<std::map<T, R> >
{
    static void encode(BinEncoder& en_, const std::map<T, R>& val_)
    {
        uint32_t num = val_.size();
        en_.copy_value((const char*)(&num), sizeof(num));
        typename std::map<T, R>::const_iterator it = val_.begin();
        for (; it != val_.end(); ++it)
        {
            en_ << it->first << it->second;
        }
    }
    static void decode(BinDecoder& de_, std::map<T, R>& val_)
    {
        uint32_t num = 0;
        de_.copy_value((void*)(&num), sizeof(num));
        for (uint32_t i = 0; i < num; ++i)
        {
            T tmp_key;
            R tmp_val;
            de_ >> tmp_key >> tmp_val;
            val_[tmp_key] = tmp_val;
        }
    }
};

GEN_CODE_ENCODE_DECODE(bool)
GEN_CODE_ENCODE_DECODE(int8_t)
GEN_CODE_ENCODE_DECODE(uint8_t)
GEN_CODE_ENCODE_DECODE_SHORT(int16_t)
GEN_CODE_ENCODE_DECODE_SHORT(uint16_t)
GEN_CODE_ENCODE_DECODE_LONG(int32_t)
GEN_CODE_ENCODE_DECODE_LONG(uint32_t)
GEN_CODE_ENCODE_DECODE_64(int64_t)
GEN_CODE_ENCODE_DECODE_64(uint64_t)

template<typename T>
BinEncoder& BinEncoder::operator << (const T& val_)
{
    CodecTool<T>::encode(*this, val_);
    return *this;
}
template<typename T>
BinDecoder& BinDecoder::operator >> (T& val_)
{
    CodecTool<T>::decode(*this, val_);
    return *this;
}

class msg_i : public Codec
{
public:
    virtual ~msg_i(){}
    msg_i(const char* msg_name_):
        m_msg_name(msg_name_)
    {}
    const char* getTypeName()  const
    {
        return m_msg_name;
    }
    BinEncoder& encoder()
    {
        return m_encoder;
    }
    BinDecoder& decoder()
    {
        return m_decoder;
    }
    virtual std::string encode_data()
    {
        m_encoder.clear();
        encode();
        return m_encoder.get_buff();
    }
    virtual void decode_data(const std::string& src_buff_)
    {
        m_decoder.init(src_buff_);
        decode();
    }
    virtual void encode()                      = 0;
    virtual void decode()                      = 0;
    const char*   m_msg_name;
    BinEncoder m_encoder;
    BinDecoder m_decoder;
};

struct FFThriftCodecTool
{
    static void write(msg_i& msg, ::apache::thrift::protocol::TProtocol* iprot);
    static void read(msg_i& msg, ::apache::thrift::protocol::TProtocol* iprot);
};

template<typename T>
class FFMsg: public msg_i
{
public:
    FFMsg():
        msg_i(TYPE_NAME(T).c_str())
    {}
    virtual ~FFMsg(){}
    void write(::apache::thrift::protocol::TProtocol* iprot)
    {
        std::string tmp = this->encode_data();
        FFThriftCodecTool::write(*this, iprot);
    }
    void read(::apache::thrift::protocol::TProtocol* iprot)
    {
        FFThriftCodecTool::read(*this, iprot);
    }
};

}



#endif

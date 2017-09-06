#ifndef _THRIFT_PROTOCOL_TBINARYPROTOCOL_H_
#define _THRIFT_PROTOCOL_TBINARYPROTOCOL_H_ 1

#include "TProtocol.h"
#include "TVirtualProtocol.h"
#include <limits>
//#include <boost/shared_ptr.hpp>

namespace apache { namespace thrift { namespace protocol {

/**
 * The default binary protocol for thrift. Writes all data in a very basic
 * binary format, essentially just spitting out the raw bytes.
 *
 */
template <class Transport_>
class TBinaryProtocolT
  : public TVirtualProtocol< TBinaryProtocolT<Transport_> > {
 protected:
  static const int32_t VERSION_MASK = ((int32_t)0xffff0000);
  static const int32_t VERSION_1 = ((int32_t)0x80010000);
  // VERSION_2 (0x80020000)  is taken by TDenseProtocol.

 public:
  TBinaryProtocolT(Transport_* trans) :
    TVirtualProtocol< TBinaryProtocolT<Transport_> >(trans),
    trans_(trans),
    string_limit_(0),
    container_limit_(0),
    strict_read_(false),
    strict_write_(true),
    string_buf_(NULL),
    string_buf_size_(0) {}

  TBinaryProtocolT(Transport_* trans,
                   int32_t string_limit,
                   int32_t container_limit,
                   bool strict_read,
                   bool strict_write) :
    TVirtualProtocol< TBinaryProtocolT<Transport_> >(trans),
    trans_(trans),
    string_limit_(string_limit),
    container_limit_(container_limit),
    strict_read_(strict_read),
    strict_write_(strict_write),
    string_buf_(NULL),
    string_buf_size_(0) {}

  ~TBinaryProtocolT() {
    if (string_buf_ != NULL) {
      std::free(string_buf_);
      string_buf_size_ = 0;
    }
  }

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setContainerSizeLimit(int32_t container_limit) {
    container_limit_ = container_limit;
  }

  void setStrict(bool strict_read, bool strict_write) {
    strict_read_ = strict_read;
    strict_write_ = strict_write;
  }

  /**
   * Writing functions.
   */

  /*ol*/ uint32_t writeMessageBegin(const std::string& name,
                                    const TMessageType messageType,
                                    const int32_t seqid);

  /*ol*/ uint32_t writeMessageEnd();


  inline uint32_t writeStructBegin(const char* name);

  inline uint32_t writeStructEnd();

  inline uint32_t writeFieldBegin(const char* name,
                                  const TType fieldType,
                                  const int16_t fieldId);

  inline uint32_t writeFieldEnd();

  inline uint32_t writeFieldStop();

  inline uint32_t writeMapBegin(const TType keyType,
                                const TType valType,
                                const uint32_t size);

  inline uint32_t writeMapEnd();

  inline uint32_t writeListBegin(const TType elemType, const uint32_t size);

  inline uint32_t writeListEnd();

  inline uint32_t writeSetBegin(const TType elemType, const uint32_t size);

  inline uint32_t writeSetEnd();

  inline uint32_t writeBool(const bool value);

  inline uint32_t writeByte(const int8_t byte);

  inline uint32_t writeI16(const int16_t i16);

  inline uint32_t writeI32(const int32_t i32);

  inline uint32_t writeI64(const int64_t i64);

  inline uint32_t writeDouble(const double dub);

  template <typename StrType>
  inline uint32_t writeString(const StrType& str);

  inline uint32_t writeBinary(const std::string& str);

  /**
   * Reading functions
   */


  /*ol*/ uint32_t readMessageBegin(std::string& name,
                                   TMessageType& messageType,
                                   int32_t& seqid);

  /*ol*/ uint32_t readMessageEnd();

  inline uint32_t readStructBegin(std::string& name);

  inline uint32_t readStructEnd();

  inline uint32_t readFieldBegin(std::string& name,
                                 TType& fieldType,
                                 int16_t& fieldId);

  inline uint32_t readFieldEnd();

  inline uint32_t readMapBegin(TType& keyType,
                               TType& valType,
                               uint32_t& size);

  inline uint32_t readMapEnd();

  inline uint32_t readListBegin(TType& elemType, uint32_t& size);

  inline uint32_t readListEnd();

  inline uint32_t readSetBegin(TType& elemType, uint32_t& size);

  inline uint32_t readSetEnd();

  inline uint32_t readBool(bool& value);
  // Provide the default readBool() implementation for std::vector<bool>
  using TVirtualProtocol< TBinaryProtocolT<Transport_> >::readBool;

  inline uint32_t readByte(int8_t& byte);

  inline uint32_t readI16(int16_t& i16);

  inline uint32_t readI32(int32_t& i32);

  inline uint32_t readI64(int64_t& i64);

  inline uint32_t readDouble(double& dub);

  template<typename StrType>
  inline uint32_t readString(StrType& str);

  inline uint32_t readBinary(std::string& str);

 protected:
  template<typename StrType>
  uint32_t readStringBody(StrType& str, int32_t sz);

  Transport_* trans_;

  int32_t string_limit_;
  int32_t container_limit_;

  // Enforce presence of version identifier
  bool strict_read_;
  bool strict_write_;

  // Buffer for reading strings, save for the lifetime of the protocol to
  // avoid memory churn allocating memory on every string read
  uint8_t* string_buf_;
  int32_t string_buf_size_;

};


template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeMessageBegin(const std::string& name,
                                                         const TMessageType messageType,
                                                         const int32_t seqid) {
  if (this->strict_write_) {
    int32_t version = (VERSION_1) | ((int32_t)messageType);
    uint32_t wsize = 0;
    wsize += writeI32(version);
    wsize += writeString(name);
    wsize += writeI32(seqid);
    return wsize;
  } else {
    uint32_t wsize = 0;
    wsize += writeString(name);
    wsize += writeByte((int8_t)messageType);
    wsize += writeI32(seqid);
    return wsize;
  }
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeMessageEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeStructBegin(const char* name) {
  (void) name;
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeStructEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeFieldBegin(const char* name,
                                                       const TType fieldType,
                                                       const int16_t fieldId) {
  (void) name;
  uint32_t wsize = 0;
  wsize += writeByte((int8_t)fieldType);
  wsize += writeI16(fieldId);
  return wsize;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeFieldEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeFieldStop() {
  return
    writeByte((int8_t)T_STOP);
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeMapBegin(const TType keyType,
                                                     const TType valType,
                                                     const uint32_t size) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t)keyType);
  wsize += writeByte((int8_t)valType);
  wsize += writeI32((int32_t)size);
  return wsize;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeMapEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeListBegin(const TType elemType,
                                                      const uint32_t size) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t) elemType);
  wsize += writeI32((int32_t)size);
  return wsize;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeListEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeSetBegin(const TType elemType,
                                                     const uint32_t size) {
  uint32_t wsize = 0;
  wsize += writeByte((int8_t)elemType);
  wsize += writeI32((int32_t)size);
  return wsize;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeSetEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeBool(const bool value) {
  uint8_t tmp =  value ? 1 : 0;
  this->trans_->write(&tmp, 1);
  return 1;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeByte(const int8_t byte) {
  this->trans_->write((uint8_t*)&byte, 1);
  return 1;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeI16(const int16_t i16) {
  int16_t net = (int16_t)htons(i16);
  this->trans_->write((uint8_t*)&net, 2);
  return 2;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeI32(const int32_t i32) {
  int32_t net = (int32_t)htonl(i32);
  this->trans_->write((uint8_t*)&net, 4);
  return 4;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeI64(const int64_t i64) {
  int64_t net = (int64_t)htonll(i64);
  this->trans_->write((uint8_t*)&net, 8);
  return 8;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeDouble(const double dub) {
  BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  uint64_t bits = bitwise_cast<uint64_t>(dub);
  bits = htonll(bits);
  this->trans_->write((uint8_t*)&bits, 8);
  return 8;
}


template <class Transport_>
template<typename StrType>
uint32_t TBinaryProtocolT<Transport_>::writeString(const StrType& str) {
  if(str.size() > static_cast<size_t>((std::numeric_limits<int32_t>::max)()))
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  uint32_t size = static_cast<uint32_t>(str.size());
  uint32_t result = writeI32((int32_t)size);
  if (size > 0) {
    this->trans_->write((uint8_t*)str.data(), size);
  }
  return result + size;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::writeBinary(const std::string& str) {
  return TBinaryProtocolT<Transport_>::writeString(str);
}

/**
 * Reading functions
 */

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readMessageBegin(std::string& name,
                                                        TMessageType& messageType,
                                                        int32_t& seqid) {
  uint32_t result = 0;
  int32_t sz;
  result += readI32(sz);

  if (sz < 0) {
    // Check for correct version number
    int32_t version = sz & VERSION_MASK;
    if (version != VERSION_1) {
      throw TProtocolException(TProtocolException::BAD_VERSION, "Bad version identifier");
    }
    messageType = (TMessageType)(sz & 0x000000ff);
    result += readString(name);
    result += readI32(seqid);
  } else {
    if (this->strict_read_) {
      throw TProtocolException(TProtocolException::BAD_VERSION, "No version identifier... old protocol client in strict mode?");
    } else {
      // Handle pre-versioned input
      int8_t type;
      result += readStringBody(name, sz);
      result += readByte(type);
      messageType = (TMessageType)type;
      result += readI32(seqid);
    }
  }
  return result;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readMessageEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readStructBegin(std::string& name) {
  name = "";
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readStructEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readFieldBegin(std::string& name,
                                                      TType& fieldType,
                                                      int16_t& fieldId) {
  (void) name;
  uint32_t result = 0;
  int8_t type;
  result += readByte(type);
  fieldType = (TType)type;
  if (fieldType == T_STOP) {
    fieldId = 0;
    return result;
  }
  result += readI16(fieldId);
  return result;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readFieldEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readMapBegin(TType& keyType,
                                                    TType& valType,
                                                    uint32_t& size) {
  int8_t k, v;
  uint32_t result = 0;
  int32_t sizei;
  result += readByte(k);
  keyType = (TType)k;
  result += readByte(v);
  valType = (TType)v;
  result += readI32(sizei);
  if (sizei < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  } else if (this->container_limit_ && sizei > this->container_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
  size = (uint32_t)sizei;
  return result;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readMapEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readListBegin(TType& elemType,
                                                     uint32_t& size) {
  int8_t e;
  uint32_t result = 0;
  int32_t sizei;
  result += readByte(e);
  elemType = (TType)e;
  result += readI32(sizei);
  if (sizei < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  } else if (this->container_limit_ && sizei > this->container_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
  size = (uint32_t)sizei;
  return result;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readListEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readSetBegin(TType& elemType,
                                                    uint32_t& size) {
  int8_t e;
  uint32_t result = 0;
  int32_t sizei;
  result += readByte(e);
  elemType = (TType)e;
  result += readI32(sizei);
  if (sizei < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  } else if (this->container_limit_ && sizei > this->container_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }
  size = (uint32_t)sizei;
  return result;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readSetEnd() {
  return 0;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readBool(bool& value) {
  uint8_t b[1];
  this->trans_->readAll(b, 1);
  value = *(int8_t*)b != 0;
  return 1;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readByte(int8_t& byte) {
  uint8_t b[1];
  this->trans_->readAll(b, 1);
  byte = *(int8_t*)b;
  return 1;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readI16(int16_t& i16) {
  union bytes {
    uint8_t b[2];
    int16_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 2);
  i16 = (int16_t)ntohs(theBytes.all);
  return 2;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readI32(int32_t& i32) {
  union bytes {
    uint8_t b[4];
    int32_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 4);
  i32 = (int32_t)ntohl(theBytes.all);
  return 4;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readI64(int64_t& i64) {
  union bytes {
    uint8_t b[8];
    int64_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 8);
  i64 = (int64_t)ntohll(theBytes.all);
  return 8;
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readDouble(double& dub) {
  BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  union bytes {
    uint8_t b[8];
    uint64_t all;
  } theBytes;
  this->trans_->readAll(theBytes.b, 8);
  theBytes.all = ntohll(theBytes.all);
  dub = bitwise_cast<double>(theBytes.all);
  return 8;
}

template <class Transport_>
template<typename StrType>
uint32_t TBinaryProtocolT<Transport_>::readString(StrType& str) {
  uint32_t result;
  int32_t size;
  result = readI32(size);
  return result + readStringBody(str, size);
}

template <class Transport_>
uint32_t TBinaryProtocolT<Transport_>::readBinary(std::string& str) {
  return TBinaryProtocolT<Transport_>::readString(str);
}

template <class Transport_>
template<typename StrType>
uint32_t TBinaryProtocolT<Transport_>::readStringBody(StrType& str,
                                                      int32_t size) {
  uint32_t result = 0;

  // Catch error cases
  if (size < 0) {
    throw TProtocolException(TProtocolException::NEGATIVE_SIZE);
  }
  if (this->string_limit_ > 0 && size > this->string_limit_) {
    throw TProtocolException(TProtocolException::SIZE_LIMIT);
  }

  // Catch empty string case
  if (size == 0) {
    str.clear();
    return result;
  }

  // Try to borrow first
  const uint8_t* borrow_buf;
  uint32_t got = size;
  if ((borrow_buf = this->trans_->borrow(NULL, &got))) {
    str.assign((const char*)borrow_buf, size);
    this->trans_->consume(size);
    return size;
  }

  // Use the heap here to prevent stack overflow for v. large strings
  if (size > this->string_buf_size_ || this->string_buf_ == NULL) {
    void* new_string_buf = std::realloc(this->string_buf_, (uint32_t)size);
    if (new_string_buf == NULL) {
      throw std::bad_alloc();
    }
    this->string_buf_ = (uint8_t*)new_string_buf;
    this->string_buf_size_ = size;
  }
  this->trans_->readAll(this->string_buf_, size);
  str.assign((char*)this->string_buf_, size);
  return (uint32_t)size;
}

typedef TBinaryProtocolT<TTransport> TBinaryProtocol;

}}} // apache::thrift::protocol

//#include "TBinaryProtocol.tcc"

#endif // #ifndef _THRIFT_PROTOCOL_TBINARYPROTOCOL_H_

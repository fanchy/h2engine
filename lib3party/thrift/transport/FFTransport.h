#ifndef _THRIFT_FFTRANSPORT_H_
#define _THRIFT_FFTRANSPORT_H_ 1

#include<stddef.h>
#include <thrift/transport/TTransport.h>
#include <thrift/transport/TVirtualTransport.h>

namespace apache { namespace thrift { namespace transport {

class FFTMemoryBuffer: public TTransport
{
    uint8_t* rBase_;
    uint8_t* rBound_;//! Ω·Œ≤
    std::string*  m_wbuff;
    const std::string*  m_rbuff;
public:
    //! for read
    FFTMemoryBuffer(const char* pbuff, uint32_t len):
        rBase_((uint8_t*)pbuff),
        rBound_((uint8_t*)rBase_ + len),
        m_wbuff(NULL),
        m_rbuff(NULL)
    {
    }
    FFTMemoryBuffer(const std::string& pstr):
        rBase_((uint8_t*)pstr.c_str()),
        rBound_((uint8_t*)rBase_ + pstr.length()),
        m_wbuff(NULL),
        m_rbuff(&pstr)
    {
    }

    FFTMemoryBuffer(std::string* pstr):
        rBase_(NULL),
        rBound_(NULL),
        m_wbuff(pstr),
        m_rbuff(NULL)
    {
    }

    bool isOpen() {
        return true;
    }
    inline std::string& get_wbuff() { return *m_wbuff; }
    inline const std::string& get_rbuff() { return *m_rbuff; }
    void open() {}
    void close() {}
    uint32_t read(uint8_t* buf, uint32_t len) {
        uint8_t* new_rBase = rBase_ + len;
        if (TDB_LIKELY(new_rBase <= rBound_)) {
            std::memcpy(buf, rBase_, len);
            rBase_ = new_rBase;
            return len;
        }
        return 0;
    }
    void write(const uint8_t* buf, uint32_t len) {
        get_wbuff().append((const char*)buf, len);
    }
    const uint8_t* borrow_virt(uint8_t* buf, uint32_t* len) {
        if (TDB_LIKELY(static_cast<ptrdiff_t>(*len) <= rBound_ - rBase_)) {
          *len = static_cast<uint32_t>(rBound_ - rBase_);
          return rBase_;
        }
        return NULL;
    }

    void consume_virt(uint32_t len) {
        if (TDB_LIKELY(static_cast<ptrdiff_t>(len) <= rBound_ - rBase_)) {
          rBase_ += len;
        } else {
          throw TTransportException(TTransportException::BAD_ARGS,
                                    "consume did not follow a borrow.");
        }
    }
};

}}}
#endif

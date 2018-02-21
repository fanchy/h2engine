#ifndef _SOCKET_I_
#define _SOCKET_I_

#include <string>


#include "netbase.h"

namespace ff {

class SocketCtrlI;

class SocketI: public Fd
{
public:
    SocketI():
        m_data(NULL) {}
    virtual ~SocketI(){}

    virtual void open() = 0;
    virtual void asyncSend(const std::string& buff_) = 0;
    virtual void asyncRecv() = 0;
    virtual void safeDelete() { delete this; }
    virtual void set_data(void* p) { m_data = p; }
    template<typename T>
    T* get_data() const { return (T*)m_data; }
    
    virtual SocketCtrlI* get_sc() = 0;
    
    virtual void sendRaw(const std::string& buff_) = 0;
private:
    void*   m_data;
};

typedef SocketI*  SocketPtr;

}

#endif

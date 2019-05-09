#ifndef _SOCKET_I_
#define _SOCKET_I_

#include <string>

#include "base/smart_ptr.h"
#include "netbase.h"

namespace ff {

class SocketCtrlI;

class SocketPrivateData{
public:
    virtual ~SocketPrivateData(){}
    virtual void* data() const = 0 ;
};
template<typename T>
class SocketDataCommon: public SocketPrivateData{
public:
    SocketDataCommon(){
        m_pData = new T();
    }
    ~SocketDataCommon(){
        delete m_pData;
        m_pData = NULL;
    }
    virtual void* data() const { return m_pData; }
    T* getData() const { return m_pData; }
    T* m_pData;
};
class SocketI: public Fd
{
public:
    SocketI() {}
    virtual ~SocketI(){
        for (std::map<std::string, SocketPrivateData*>::iterator it = m_dataPrivate.begin(); it != m_dataPrivate.end(); ++it)
        {
            delete it->second;
        }
        m_dataPrivate.clear();
    }

    virtual void open() = 0;
    virtual void asyncSend(const std::string& buff_) = 0;
    virtual void asyncRecv() = 0;
    template<typename T>
    T* getData() {
        SocketDataCommon<T>* pRet = NULL;
        std::map<std::string, SocketPrivateData*>::iterator it = m_dataPrivate.find(TYPE_NAME(T));
        if (it == m_dataPrivate.end()){
            pRet = new SocketDataCommon<T>();
            m_dataPrivate[TYPE_NAME(T)] = pRet;
        }
        else{
            pRet = (SocketDataCommon<T>*)(it->second);
        }
        return pRet->getData();
    }

    virtual SocketCtrlI* getSocketCtrl() = 0;

    virtual void sendRaw(const std::string& buff_) = 0;
    virtual SharedPtr<SocketI> toSharedPtr() = 0;
    virtual void refSelf(SharedPtr<SocketI> p) = 0;
private:
    std::map<std::string, SocketPrivateData*>    m_dataPrivate;
};

typedef SharedPtr<SocketI>  SocketObjPtr;
typedef SocketI*  SocketPtr;

}

#endif

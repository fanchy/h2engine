#ifndef _SOCKET_I_
#define _SOCKET_I_

#include <string>
#include <map>

#include "base/fftype.h"
#include "base/smart_ptr.h"
#include "base/anytype.h"
#include "base/func.h"

namespace ff {

enum SocketTypeDef{
    SOCKET_BIN = 1,
    SOCKET_WS  = 2,
};
class SocketObj
{
public:
    SocketObj():protocolType(0) {}
    virtual ~SocketObj(){}

    virtual Socketfd getRawSocket() = 0;

    virtual void open() = 0;
    virtual void close()= 0;

    virtual void sendRaw(const std::string& data) = 0;
    virtual void asyncSend(const std::string& data){
        if (funcBuildPkg){
            sendRaw(funcBuildPkg(data));
        }
        else{
            sendRaw(data);
        }
    }

    template<typename T>
    T& getData() {
        return data.getData<T>();
    }

    virtual SharedPtr<SocketObj> toSharedPtr() = 0;
    virtual void refSelf(SharedPtr<SocketObj> p) = 0;
public:
    int                                             protocolType;
    AnyType                                         data;
    Function<std::string(const std::string& data)>  funcBuildPkg; 
};

typedef SharedPtr<SocketObj>  SocketObjPtr;

typedef Function<void(SocketObjPtr, int, const char*, size_t)> SocketEventFunc;
}

#endif

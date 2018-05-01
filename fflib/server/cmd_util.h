#ifndef _FF_CMD_UTIL_H_
#define _FF_CMD_UTIL_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "base/smart_ptr.h"
#include "base/event_bus.h"
#include "server/entity.h"
#include "thrift/FFThrift.h"

namespace ff{

enum LogoutDef{
    LOGOUT_CMD = 0xFFFF
};
class CmdHandler{
public:
    virtual ~CmdHandler(){}
    virtual void handleCmd(EntityPtr entity, uint16_t cmd, const std::string& data) = 0;
};
typedef SharedPtr<CmdHandler> CmdHandlerPtr;

template <typename T>
class CmdHandlerFunc: public CmdHandler
{
public:
    CmdHandlerFunc(T pFuncArg):pFunc(pFuncArg){}
    virtual void handleCmd(EntityPtr entity, uint16_t cmd, const std::string& data)
    {
        pFunc(entity, data);
    }
    T pFunc;
};


template <typename T>
struct CmdFunctor;

template <typename T>
struct CmdFunctor{
    typedef void (*CMD_FUNC)(EntityPtr, T&);
    CmdFunctor(CMD_FUNC f):func(f){}
    void operator()(EntityPtr e, const std::string& data){
        T retmsg;
        FFThrift::DecodeFromString(retmsg, data);
        func(e, retmsg);
    }
    
    CMD_FUNC func;
};

template <>
struct CmdFunctor<const std::string>{
    typedef void (*CMD_FUNC)(EntityPtr, const std::string&);
    CmdFunctor(CMD_FUNC f):func(f){}
    void operator()(EntityPtr e, const std::string& data){
        func(e, data);
    }
    
    CMD_FUNC func;
};


template <typename CLASS_TYPE, typename T>
struct CmdObjMethod;

template <typename CLASS_TYPE, typename T>
struct CmdObjMethod{
    typedef void (CLASS_TYPE::*CMD_FUNC)(EntityPtr, T&);
    
    CmdObjMethod(CMD_FUNC f, CLASS_TYPE* p):func(f), obj(p){}
    void operator()(EntityPtr e, const std::string& data){
        T retmsg;
        FFThrift::DecodeFromString(retmsg, data);
        (obj->*func)(e, retmsg);
    }
    
    CMD_FUNC func;
    CLASS_TYPE* obj;
};

template <typename CLASS_TYPE>
struct CmdObjMethod<CLASS_TYPE, const std::string>{
    typedef void (CLASS_TYPE::*CMD_FUNC)(EntityPtr, const std::string&);
    
    CmdObjMethod(CMD_FUNC f, CLASS_TYPE* p):func(f), obj(p){}
    void operator()(EntityPtr e, const std::string& data){
        (obj->*func)(e, data);
    }
    
    CMD_FUNC func;
    CLASS_TYPE* obj;
};

typedef bool (*CMD_HOOK_FUNCTOR)(EntityPtr, uint16_t, const std::string&);
class CmdMgr
{
public:
    CmdMgr():m_funcHook(NULL){}
    template <typename CLASS_TYPE, typename T>
    CmdMgr& reg(uint16_t cmd, void (CLASS_TYPE::*func)(EntityPtr, T&) , CLASS_TYPE* obj){
        CmdObjMethod<CLASS_TYPE, T> functor(func, obj);
        m_regCmdHandler[cmd] = new CmdHandlerFunc<CmdObjMethod<CLASS_TYPE, T> >(functor);
        return *this;
    }
    
    template <typename T>
    CmdMgr& reg(uint16_t cmd, void (*func)(EntityPtr, T&)){
        CmdFunctor<T> functor(func);
        m_regCmdHandler[cmd] = new CmdHandlerFunc<CmdFunctor<T> >(functor);
        return *this;
    }

    CmdHandlerPtr getCmdHandler(uint16_t cmd){
        std::map<uint16_t, CmdHandlerPtr>::iterator it = m_regCmdHandler.find(cmd);
        if (it != m_regCmdHandler.end()){
            return it->second;
        }
        return NULL;
    }
    
    void setCmdHookFunc(CMD_HOOK_FUNCTOR f){ 
        m_funcHook = f;
    }
public:
    CMD_HOOK_FUNCTOR                    m_funcHook;
    std::map<uint16_t, CmdHandlerPtr>   m_regCmdHandler;
};
#define CMD_MGR Singleton<CmdMgr>::instance()

struct CmdModule{
    static bool init();
};
}
#endif

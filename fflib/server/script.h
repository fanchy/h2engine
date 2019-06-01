#ifndef _FF_SCRIPT_H_
#define _FF_SCRIPT_H_


#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "base/singleton.h"
#include "base/smart_ptr.h"
#include "base/fftype.h"
#include "base/func.h"

namespace ff
{

#define SCRIPT_UTIL Singleton<ScriptUtil>::instance()
struct ScriptArgObj{
    typedef Function<void(SharedPtr<ScriptArgObj>)> ScriptFunction;
    enum ArgType{
        ARG_NULL,
        ARG_INT,
        ARG_FLOAT,
        ARG_STRING,
        ARG_LIST,
        ARG_DICT,
        ARG_FUNC,
    };
    ScriptArgObj():nType(ARG_NULL), nVal(0), fVal(0.0){}
    ScriptArgObj(int64_t v):nType(ARG_INT), nVal(v), fVal(0.0){}
    ScriptArgObj(int8_t v):nType(ARG_INT), nVal(v), fVal(0.0){}
    ScriptArgObj(int16_t v):nType(ARG_INT), nVal(v), fVal(0.0){}
    ScriptArgObj(int32_t v):nType(ARG_INT), nVal(v), fVal(0.0){}
    ScriptArgObj(uint8_t v):nType(ARG_INT), nVal(v), fVal(0.0){}
    ScriptArgObj(uint16_t v):nType(ARG_INT), nVal(v), fVal(0.0){}
    ScriptArgObj(uint32_t v):nType(ARG_INT), nVal(v), fVal(0.0){}
    ScriptArgObj(bool v):nType(ARG_INT), nVal(0), fVal(0.0){if (v) nVal = 1;}
    ScriptArgObj(double v):nType(ARG_FLOAT), nVal(0), fVal(v){}
    ScriptArgObj(const std::string& v):nType(ARG_STRING), nVal(0), fVal(0.0), sVal(v){}
    ScriptArgObj(ScriptFunction f):nType(ARG_FUNC), nVal(0), fVal(0.0),func(f){}

    template<typename T>
    ScriptArgObj(T* v):nType(ARG_INT), nVal((int64_t)v), fVal(0.0){}
    template<typename T>
    ScriptArgObj(const T* v):nType(ARG_INT), nVal((int64_t)v), fVal(0.0){}
    template<typename T>
    ScriptArgObj(SharedPtr<T> v):nType(ARG_INT), nVal((int64_t)(SMART_PTR_RAW(v))), fVal(0.0){}

    static SharedPtr<ScriptArgObj> create() { return new ScriptArgObj(); }
    template<typename T>
    static SharedPtr<ScriptArgObj> create(T v) { return new ScriptArgObj(v); }

    void toNull(){
        nType = ARG_NULL;
    }
    void toInt(int64_t v){
        nType = ARG_INT;
        nVal  = v;
    }
    void toFloat(double v){
        nType = ARG_FLOAT;
        fVal  = v;
    }
    void toString(const std::string& v){
        nType = ARG_STRING;
        sVal  = v;
    }
    void toList(std::vector<SharedPtr<ScriptArgObj> >* pVal = NULL){
        nType = ARG_LIST;
        if (pVal){
            listVal = *pVal;
        }
    }
    void toDict(std::map<std::string, SharedPtr<ScriptArgObj> >* pVal = NULL){
        nType = ARG_DICT;
        if (pVal){
            dictVal = *pVal;
        }
    }
    void toFunc(ScriptFunction f){
        nType = ARG_FUNC;
        func = f;
    }
    void dump(int depth = 0){
        char buff[64] = {0};
        snprintf(buff, sizeof(buff), "%%%ds", depth+1);
        printf(buff, "");
        if (isNull()){
            printf("Null");
        }
        else if (isInt()){
            printf("%ld(int)", long(nVal));
        }
        else if (isFloat()){
            printf("%g(float)", fVal);
        }
        else if (isString()){
            printf("%s(string)", sVal.c_str());
        }
        else if (isList()){
            printf("size=%d(list)\n", int(listVal.size()));
            for (unsigned int i = 0; i < listVal.size(); ++i){
                char buff[64] = {0};
                snprintf(buff, sizeof(buff), "%%%ds[%d]", depth+1, i);
                printf(buff, "");
                listVal[i]->dump(depth+1);
                printf("\n");
            }
        }
        else if (isDict()){
            printf("size=%d(dict)\n", int(dictVal.size()));
            std::map<std::string, SharedPtr<ScriptArgObj> >::iterator it = dictVal.begin();
            for (int i = 0; it != dictVal.end(); ++it){
                char buff[64] = {0};
                snprintf(buff, sizeof(buff), "%%%ds[%d] %s:", depth+1, i, it->first.c_str());
                printf(buff, "");
                it->second->dump(depth+1);
                printf("\n");
                ++i;
            }
        }
        printf("\n");
    }
    bool isNull() const         { return nType == ARG_NULL; }
    bool isInt() const          { return nType == ARG_INT;  }
    bool isFloat() const        { return nType == ARG_FLOAT; }
    bool isString() const       { return nType == ARG_STRING; }
    bool isList() const         { return nType == ARG_LIST; }
    bool isDict() const         { return nType == ARG_DICT; }
    bool isFunc() const         { return nType == ARG_FUNC; }

    int64_t             getInt()           {
        if (isString()){
            return ::atol(sVal.c_str());
        }
        else if (isFloat()){
            return (int64_t)fVal;
        }
        return nVal;
    }
    double              getFloat() const        { return fVal; }
    const std::string&  getString()        {
        if (isInt()){
            char buff[64] = {0};
            snprintf(buff, sizeof(buff), "%ld", long(nVal));
            toString(buff);
        }
        else if (isFloat()){
            char buff[64] = {0};
            snprintf(buff, sizeof(buff), "%g", fVal);
            toString(buff);
        }
        return sVal;
    }
    const std::vector<SharedPtr<ScriptArgObj> >& getList() const         { return listVal; }
    const std::map<std::string, SharedPtr<ScriptArgObj> >& getDict() const         { return dictVal; }
    ScriptFunction& getFunc(){ return func; }
    void copy(SharedPtr<ScriptArgObj>& src){
        nType   = src->nType;
        nVal    = src->nVal;
        fVal    = src->fVal;
        sVal    = src->sVal;
        listVal = src->listVal;
        dictVal = src->dictVal;
        func    = src->func;
    }
    int                                                      nType;
    int64_t                                                  nVal;
    double                                                   fVal;
    std::string                                              sVal;
    std::vector<SharedPtr<ScriptArgObj> >                    listVal;
    std::map<std::string, SharedPtr<ScriptArgObj> >          dictVal;
    ScriptFunction                                           func;
};
typedef SharedPtr<ScriptArgObj> ScriptArgObjPtr;
template<typename T>
struct ScriptArgUtil{
    static ScriptArgObjPtr toValue(T v){
        return ScriptArgObj::create(v);
    }
};
template<>
struct ScriptArgUtil<ScriptArgObjPtr>{
    static ScriptArgObjPtr toValue(ScriptArgObjPtr v){
        return v;
    }
};
template<typename T>
struct ScriptArgUtil<std::vector<T> >{
    static ScriptArgObjPtr toValue(const std::vector<T>& v){
        ScriptArgObjPtr ret = ScriptArgObj::create();
        std::vector<ScriptArgObjPtr> listValue;
        for (size_t i = 0; i < v.size(); ++i){
            listValue.push_back(ScriptArgUtil<T>::toValue(v[i]));
        }
        ret->toList(&listValue);
        return ret;
    }
};
template<typename T>
struct ScriptArgUtil<std::list<T> >{
    static ScriptArgObjPtr toValue(const std::list<T>& v){
        ScriptArgObjPtr ret = ScriptArgObj::create();
        std::vector<ScriptArgObjPtr> listValue;
        for (typename std::list<T>::iterator it = v.begin(); it != v.end(); ++it){
            listValue.push_back(ScriptArgUtil<T>::toValue(*it));
        }
        ret->toList(&listValue);
        return ret;
    }
};
template<typename T>
struct ScriptArgUtil<std::set<T> >{
    static ScriptArgObjPtr toValue(const std::set<T>& v){
        ScriptArgObjPtr ret = ScriptArgObj::create();
        std::vector<ScriptArgObjPtr> listValue;
        for (typename std::set<T>::iterator it = v.begin(); it != v.end(); ++it){
            listValue.push_back(ScriptArgUtil<T>::toValue(*it));
        }
        ret->toList(&listValue);
        return ret;
    }
};
template<typename K, typename V>
struct ScriptArgUtil<std::map<K, V> >{
    static ScriptArgObjPtr toValue(const std::map<K, V>& v){
        ScriptArgObjPtr ret = ScriptArgObj::create();
        std::map<std::string, ScriptArgObjPtr> mapValue;
        for (typename std::map<K, V>::iterator it = v.begin(); it != v.end(); ++it){
            ScriptArgObjPtr key = ScriptArgUtil<K>::toValue(it->first);
            mapValue[key->getString()] = ScriptArgUtil<V>::toValue(it->second);
        }
        ret->toDict(&mapValue);
        return ret;
    }
};


struct ScriptArgs{
    ScriptArgs(){
        ret = new ScriptArgObj();
    }
    void returnValue(bool v){
        int n = v? 1: 0;
        ret->toInt(n);
    }
    void returnValue(int64_t v){
        ret->toInt(v);
    }
    void returnValue(int8_t v){
        ret->toInt(v);
    }
    void returnValue(int16_t v){
        ret->toInt(v);
    }
    void returnValue(int32_t v){
        ret->toInt(v);
    }
    void returnValue(uint8_t v){
        ret->toInt(v);
    }
    void returnValue(uint16_t v){
        ret->toInt(v);
    }
    void returnValue(uint32_t v){
        ret->toInt(v);
    }
    void returnValue(double v){
        ret->toFloat(v);
    }
    void returnValue(const std::string& v){
        ret->toString(v);
    }
    void returnValue(const char* v){
        ret->toString(v);
    }
    void returnValue(ScriptArgObjPtr v){
        if (v){
            ret = v;
        }
    }
    template<typename T>
    void returnValue(T* v) { ret->toInt((int64_t)v); }
    template<typename T>
    void returnValue(const T* v) { ret->toInt((int64_t)v); }
    template<typename T>
    void returnValue(SharedPtr<T> v) { ret->toInt((int64_t)(SMART_PTR_RAW(v))); }

    void dumpArgs(){
        printf("totoal args num:%d\n", int(args.size()));
        for (unsigned int i = 0; i < args.size(); ++i){
            printf("[%u] ", i);
            args[i]->dump();
            printf("\n");
        }
        printf("\n");
    }
    ScriptArgObjPtr at(size_t n){
        if (n >= args.size()){
            ScriptArgObjPtr ret = new ScriptArgObj();
            return ret;
        }
        return args[n];
    }
    ScriptArgObjPtr atOrCreate(size_t n){
        for (size_t i = args.size(); i <= n; ++i){
            ScriptArgObjPtr ret = new ScriptArgObj();
            args.push_back(ret);
        }
        return args[n];
    }
    ScriptArgObjPtr getReturnValue(){ return ret;}
    std::vector<ScriptArgObjPtr> args;
    ScriptArgObjPtr              ret;
};

typedef void (*ScriptStaticFunctor)(ScriptArgs&);
class ScriptFunctor{
public:
    virtual ~ScriptFunctor(){}

    virtual void callFunc(ScriptArgs& args) = 0;
};

template<typename T>
struct ScriptFunctorUtil;

template<typename RET>
struct CallFuncRetUtil;//!ret type traits

class ArgDataCacheBase{
public:
    virtual ~ArgDataCacheBase(){};
};
template<typename T>
class ArgDataCacheCommon: public ArgDataCacheBase{
public:
    ArgDataCacheCommon(T& a):data(a){}
    T data;
};
struct TmpCacheArg{
    std::map<std::string, ArgDataCacheBase*> cacheData;
    ~TmpCacheArg(){
        clear();
    }
    void clear(){
        for (std::map<std::string, ArgDataCacheBase*>::iterator it = cacheData.begin(); it != cacheData.end(); ++it)
        {
            delete it->second;
        }
        cacheData.clear();
    }
};
#define gTmpCacheArg Singleton<TmpCacheArg>::instance()

template<typename T>
struct CppScriptValutil{
    static void toScriptVal(ScriptArgObjPtr retVal, T& a){
        char strAddr[256] = {0};
        static unsigned int addr = 0;
        snprintf(strAddr, sizeof(strAddr), "%s#%d", TYPE_NAME(T).c_str(), ++addr);
        gTmpCacheArg.cacheData[strAddr] = new ArgDataCacheCommon<T>(a);
        retVal->toString(strAddr);
    }
    static void toCppVal(ScriptArgObjPtr argVal, T& a){
        std::string strAddr = argVal->getString();
        std::string strTypeName = TYPE_NAME(T);
        strTypeName += "#";
        if (strAddr.find(strTypeName) == std::string::npos){
            return;
        }
        a = ((ArgDataCacheCommon<T>*)gTmpCacheArg.cacheData[strAddr])->data;
    }
};

typedef bool (*CallScriptFunctor)(const std::string&, ScriptArgs&);
class ScriptUtil{
public:
    ScriptUtil():m_funcCallScript(NULL), m_flagCallScript(0){}
    ~ScriptUtil(){
        std::map<std::string/*funcName*/, ScriptFunctor*>::iterator  it = m_functors.begin();
        for (; it != m_functors.end(); ++it){
            delete it->second;
        }
        m_functors.clear();
    }
    template<typename F>
    ScriptUtil& reg(const std::string& funcName, F f){
        typedef typename ScriptFunctorUtil<F>::ScriptFunctorImpl FunctorImpl;
        m_functors[funcName] = new FunctorImpl(f);
        return *this;
    }
    template<typename F, typename O>
    ScriptUtil& reg(const std::string& funcName, F f, O* obj){
        typedef typename ScriptFunctorUtil<F>::ScriptFunctorImpl FunctorImpl;
        m_functors[funcName] = new FunctorImpl(f, obj);
        return *this;
    }
    ScriptFunctor* find(const std::string& funcName){
        std::map<std::string/*funcName*/, ScriptFunctor*>::iterator it = m_functors.find(funcName);
        if (it != m_functors.end()){
            return it->second;
        }
        return NULL;
    }
    bool callFunc(const std::string& funcName, ScriptArgs& scriptArg){
        ScriptFunctor* f = this->find(funcName);
        if (f){
            f->callFunc(scriptArg);
            return true;
        }
        return false;
    }

    //!-----call script
    void setCallScriptFunc(CallScriptFunctor f){
        m_funcCallScript = f;
    }
    bool isExceptEnable() const { return m_flagCallScript != 0;}
    void setExceptEnable(int flag) { m_flagCallScript = flag;}

    template<typename RET>
    typename CallFuncRetUtil<RET>::RET_TYPE callScriptRaw(const std::string& funcName, ScriptArgs& scriptArg){
        typename CallFuncRetUtil<RET>::RET_TYPE ret = TypeInitValUtil<typename CallFuncRetUtil<RET>::RET_TYPE>::initVal();
        if (m_funcCallScript){
            (*m_funcCallScript)(funcName, scriptArg);
            CppScriptValutil<typename CallFuncRetUtil<RET>::RET_TYPE>::toCppVal(scriptArg.getReturnValue(), ret);
        }
        else{
            printf("callScriptRaw none script impl functor\n");
        }
        gTmpCacheArg.clear();
        return ret;
    }
    template<typename RET>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName){
        ScriptArgs varScript;
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1, typename ARG2>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1, ARG2& arg2){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        CppScriptValutil<ARG2>::toScriptVal(varScript.atOrCreate(1), arg2);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1, ARG2& arg2,
                ARG3& arg3){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        CppScriptValutil<ARG2>::toScriptVal(varScript.atOrCreate(1), arg2);
        CppScriptValutil<ARG3>::toScriptVal(varScript.atOrCreate(2), arg3);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1, ARG2& arg2,
                ARG3& arg3, ARG4& arg4){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        CppScriptValutil<ARG2>::toScriptVal(varScript.atOrCreate(1), arg2);
        CppScriptValutil<ARG3>::toScriptVal(varScript.atOrCreate(2), arg3);
        CppScriptValutil<ARG4>::toScriptVal(varScript.atOrCreate(3), arg4);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1, ARG2& arg2,
                ARG3& arg3, ARG4& arg4, ARG5& arg5){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        CppScriptValutil<ARG2>::toScriptVal(varScript.atOrCreate(1), arg2);
        CppScriptValutil<ARG3>::toScriptVal(varScript.atOrCreate(2), arg3);
        CppScriptValutil<ARG4>::toScriptVal(varScript.atOrCreate(3), arg4);
        CppScriptValutil<ARG5>::toScriptVal(varScript.atOrCreate(4), arg5);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
                           typename ARG6>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1, ARG2& arg2,
                ARG3& arg3, ARG4& arg4, ARG5& arg5, ARG6& arg6){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        CppScriptValutil<ARG2>::toScriptVal(varScript.atOrCreate(1), arg2);
        CppScriptValutil<ARG3>::toScriptVal(varScript.atOrCreate(2), arg3);
        CppScriptValutil<ARG4>::toScriptVal(varScript.atOrCreate(3), arg4);
        CppScriptValutil<ARG5>::toScriptVal(varScript.atOrCreate(4), arg5);
        CppScriptValutil<ARG6>::toScriptVal(varScript.atOrCreate(5), arg6);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
                           typename ARG6, typename ARG7>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1, ARG2& arg2,
                ARG3& arg3, ARG4& arg4, ARG5& arg5, ARG6& arg6, ARG7& arg7){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        CppScriptValutil<ARG2>::toScriptVal(varScript.atOrCreate(1), arg2);
        CppScriptValutil<ARG3>::toScriptVal(varScript.atOrCreate(2), arg3);
        CppScriptValutil<ARG4>::toScriptVal(varScript.atOrCreate(3), arg4);
        CppScriptValutil<ARG5>::toScriptVal(varScript.atOrCreate(4), arg5);
        CppScriptValutil<ARG6>::toScriptVal(varScript.atOrCreate(5), arg6);
        CppScriptValutil<ARG7>::toScriptVal(varScript.atOrCreate(6), arg7);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
                           typename ARG6, typename ARG7, typename ARG8>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1, ARG2& arg2,
                ARG3& arg3, ARG4& arg4, ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        CppScriptValutil<ARG2>::toScriptVal(varScript.atOrCreate(1), arg2);
        CppScriptValutil<ARG3>::toScriptVal(varScript.atOrCreate(2), arg3);
        CppScriptValutil<ARG4>::toScriptVal(varScript.atOrCreate(3), arg4);
        CppScriptValutil<ARG5>::toScriptVal(varScript.atOrCreate(4), arg5);
        CppScriptValutil<ARG6>::toScriptVal(varScript.atOrCreate(5), arg6);
        CppScriptValutil<ARG7>::toScriptVal(varScript.atOrCreate(6), arg7);
        CppScriptValutil<ARG8>::toScriptVal(varScript.atOrCreate(7), arg8);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
    template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5,
                           typename ARG6, typename ARG7, typename ARG8, typename ARG9>
    typename CallFuncRetUtil<RET>::RET_TYPE callScript(const std::string& funcName, ARG1& arg1, ARG2& arg2,
                ARG3& arg3, ARG4& arg4, ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8, ARG9& arg9){
        ScriptArgs varScript;
        CppScriptValutil<ARG1>::toScriptVal(varScript.atOrCreate(0), arg1);
        CppScriptValutil<ARG2>::toScriptVal(varScript.atOrCreate(1), arg2);
        CppScriptValutil<ARG3>::toScriptVal(varScript.atOrCreate(2), arg3);
        CppScriptValutil<ARG4>::toScriptVal(varScript.atOrCreate(3), arg4);
        CppScriptValutil<ARG5>::toScriptVal(varScript.atOrCreate(4), arg5);
        CppScriptValutil<ARG6>::toScriptVal(varScript.atOrCreate(5), arg6);
        CppScriptValutil<ARG7>::toScriptVal(varScript.atOrCreate(6), arg7);
        CppScriptValutil<ARG8>::toScriptVal(varScript.atOrCreate(7), arg8);
        CppScriptValutil<ARG9>::toScriptVal(varScript.atOrCreate(8), arg9);
        return callScriptRaw<typename CallFuncRetUtil<RET>::RET_TYPE>(funcName, varScript);
    }
public:
    std::map<std::string/*funcName*/, ScriptFunctor*>    m_functors;
    CallScriptFunctor                                    m_funcCallScript;
    int                                                  m_flagCallScript;//!调用脚本出错后，是否抛出异常
};

//************************************************************detail
template<>
struct ScriptFunctorUtil<void (*)(ScriptArgs&)>{
    typedef void (*DestFunc)(ScriptArgs&);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                (*destFunc)(args);
            }
        }
        DestFunc destFunc;
    };
};
template<>
struct CppScriptValutil<int8_t>{
    static void toScriptVal(ScriptArgObjPtr retVal, int8_t a){
        retVal->toInt(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, int8_t& a){
        a = (int8_t)argVal->getInt();
    }
};
template<>
struct CppScriptValutil<uint8_t>{
    static void toScriptVal(ScriptArgObjPtr retVal, uint8_t a){
        retVal->toInt(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, uint8_t& a){
        a = (uint8_t)argVal->getInt();
    }
};
template<>
struct CppScriptValutil<int16_t>{
    static void toScriptVal(ScriptArgObjPtr retVal, int16_t a){
        retVal->toInt(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, int16_t& a){
        a = (int16_t)argVal->getInt();
    }
};
template<>
struct CppScriptValutil<uint16_t>{
    static void toScriptVal(ScriptArgObjPtr retVal, uint16_t a){
        retVal->toInt(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, uint16_t& a){
        a = (uint16_t)argVal->getInt();
    }
};
template<>
struct CppScriptValutil<int32_t>{
    static void toScriptVal(ScriptArgObjPtr retVal, int32_t a){
        retVal->toInt(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, int32_t& a){
        a = (int32_t)argVal->getInt();
    }
};
template<>
struct CppScriptValutil<uint32_t>{
    static void toScriptVal(ScriptArgObjPtr retVal, uint32_t a){
        retVal->toInt(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, uint32_t& a){
        a = (uint32_t)argVal->getInt();
    }
};
template<>
struct CppScriptValutil<int64_t>{
    static void toScriptVal(ScriptArgObjPtr retVal, int64_t a){
        retVal->toInt(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, int64_t& a){
        a = (int64_t)argVal->getInt();
    }
};
template<>
struct CppScriptValutil<uint64_t>{
    static void toScriptVal(ScriptArgObjPtr retVal, uint64_t a){
        retVal->toInt(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, uint64_t& a){
        a = (uint64_t)argVal->getInt();
    }
};
template<>
struct CppScriptValutil<std::string>{
    static void toScriptVal(ScriptArgObjPtr retVal, const std::string& a){
        retVal->toString(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, std::string& a){
        a = argVal->getString();
    }
};
template<>
struct CppScriptValutil<double>{
    static void toScriptVal(ScriptArgObjPtr retVal, double a){
        retVal->toFloat(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, double& a){
        a = argVal->getFloat();
    }
};
template<>
struct CppScriptValutil<char*>{
    static void toScriptVal(ScriptArgObjPtr retVal, const char* a){
        retVal->toString(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, char* &a){
        a = (char*)(argVal->getString().c_str());
    }
};
template<typename T>
struct CppScriptValutil<T*>{
    static void toScriptVal(ScriptArgObjPtr retVal, const T* a){
        retVal->toInt((long)a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, T* &a){
        a = (T*)(argVal->getInt());
    }
};
template<>
struct CppScriptValutil<ScriptArgObjPtr>{
    static void toScriptVal(ScriptArgObjPtr retVal, ScriptArgObjPtr& a){
        retVal->copy(a);
    }
    static void toCppVal(ScriptArgObjPtr argVal, ScriptArgObjPtr &a){
        a = argVal;
    }
};
template<typename T>
struct CppScriptValutil<std::vector<T> >{
    static void toScriptVal(ScriptArgObjPtr retVal, const std::vector<T>& a){
        retVal->toList();
        for (size_t i = 0; i < a.size(); ++i){
            ScriptArgObjPtr val = new ScriptArgObj();
            CppScriptValutil<T>::toScriptVal(val, a[i]);
            retVal->listVal.push_back(val);
        }
    }
    static void toCppVal(ScriptArgObjPtr argVal, std::vector<T> &a){
        for (size_t i = 0; i < argVal->listVal.size(); ++i){
            T val;
            CppScriptValutil<T>::toCppVal(argVal->listVal[i], val);
            a.push_back(val);
        }
    }
};
template<typename T>
struct CppScriptValutil<std::list<T> >{
    static void toScriptVal(ScriptArgObjPtr retVal, const std::list<T>& a){
        retVal->toList();
        for (typename std::list<T>::const_iterator it = a.begin(); it != a.end(); ++it){
            ScriptArgObjPtr val = new ScriptArgObj();
            CppScriptValutil<T>::toScriptVal(val, *it);
            retVal->listVal.push_back(val);
        }
    }
    static void toCppVal(ScriptArgObjPtr argVal, std::list<T> &a){
        for (size_t i = 0; i < argVal->listVal.size(); ++i){
            T val;
            CppScriptValutil<T>::toCppVal(argVal->listVal[i], val);
            a.push_back(val);
        }
    }
};
template<typename T>
struct CppScriptValutil<std::set<T> >{
    static void toScriptVal(ScriptArgObjPtr retVal, const std::set<T>& a){
        retVal->toList();
        for (typename std::set<T>::const_iterator it = a.begin(); it != a.end(); ++it){
            ScriptArgObjPtr val = new ScriptArgObj();
            CppScriptValutil<T>::toScriptVal(val, *it);
            retVal->listVal.push_back(val);
        }
    }
    static void toCppVal(ScriptArgObjPtr argVal, std::set<T> &a){
        for (size_t i = 0; i < argVal->listVal.size(); ++i){
            T val;
            CppScriptValutil<T>::toCppVal(argVal->listVal[i], val);
            a.insert(val);
        }
    }
};
template<typename T, typename R>
struct CppScriptValutil<std::map<T, R> >{
    static void toScriptVal(ScriptArgObjPtr retVal, const std::map<T, R>& a){
        retVal->toDict();
        for (typename std::map<T, R>::const_iterator it = a.begin(); it != a.end(); ++it){
            ScriptArgObjPtr key = new ScriptArgObj();
            ScriptArgObjPtr val = new ScriptArgObj();
            CppScriptValutil<T>::toScriptVal(key, it->first);
            CppScriptValutil<T>::toScriptVal(val, it->second);
            retVal->dictVal[key->getString()] = val;
        }
    }
    static void toCppVal(ScriptArgObjPtr argVal, std::map<T, R> &a){
        std::map<std::string, ScriptArgObjPtr>::iterator it = argVal->dictVal.begin();
        for (; it != argVal->dictVal.end(); ++it){
            T key;
            R val;
            ScriptArgObjPtr keyTmp = new ScriptArgObj(it->first);
            CppScriptValutil<T>::toCppVal(keyTmp, key);
            CppScriptValutil<T>::toCppVal(it->second, val);
            a[key] = val;
        }
    }
};


template<typename RET>
struct CallFuncRetUtil{
    typedef RET RET_TYPE;
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8, ARG9& arg9){
        RET ret = (*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7, typename ARG8>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8){
        RET ret = (*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7){
        RET ret = (*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6){
        RET ret = (*f)(arg1, arg2, arg3, arg4, arg5, arg6);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5){
        RET ret = (*f)(arg1, arg2, arg3, arg4, arg5);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4){
        RET ret = (*f)(arg1, arg2, arg3, arg4);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3){
        RET ret = (*f)(arg1, arg2, arg3);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC, typename ARG1, typename ARG2>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2){
        RET ret = (*f)(arg1, arg2);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC, typename ARG1>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1){
        RET ret = (*f)(arg1);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC>
    static void call(ScriptArgs& args, FUNC f){
        RET ret = (*f)();
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }

    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8, ARG9& arg9){
        RET ret = (obj->*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7, typename ARG8>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8){
        RET ret = (obj->*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7){
        RET ret = (obj->*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6){
        RET ret = (obj->*f)(arg1, arg2, arg3, arg4, arg5, arg6);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5){
        RET ret = (obj->*f)(arg1, arg2, arg3, arg4, arg5);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4){
        RET ret = (obj->*f)(arg1, arg2, arg3, arg4);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3){
        RET ret = (obj->*f)(arg1, arg2, arg3);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2){
        RET ret = (obj->*f)(arg1, arg2);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1){
        RET ret = (obj->*f)(arg1);
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
    template<typename FUNC_CLASS_TYPE, typename FUNC>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f){
        RET ret = (obj->*f)();
        CppScriptValutil<RET>::toScriptVal(args.getReturnValue(), ret);
    }
};
template<>
struct CallFuncRetUtil<void>{
    typedef int RET_TYPE;
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8, ARG9& arg9){
        (*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);

    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7, typename ARG8>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8){
        (*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);

    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7){
        (*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);

    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6){
        (*f)(arg1, arg2, arg3, arg4, arg5, arg6);

    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5){
        (*f)(arg1, arg2, arg3, arg4, arg5);

    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4){
        (*f)(arg1, arg2, arg3, arg4);

    }
    template<typename FUNC, typename ARG1, typename ARG2, typename ARG3>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3){
        (*f)(arg1, arg2, arg3);

    }
    template<typename FUNC, typename ARG1, typename ARG2>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2){
        (*f)(arg1, arg2);

    }
    template<typename FUNC, typename ARG1>
    static void call(ScriptArgs& args, FUNC f, ARG1& arg1){
        (*f)(arg1);

    }
    template<typename FUNC>
    static void call(ScriptArgs& args, FUNC f){
        (*f)();
    }

    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8, ARG9& arg9){
        (obj->*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7, typename ARG8>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7, ARG8& arg8){
        (obj->*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6, typename ARG7>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6, ARG7& arg7){
        (obj->*f)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5, typename ARG6>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5, ARG6& arg6){
        (obj->*f)(arg1, arg2, arg3, arg4, arg5, arg6);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
             typename ARG5>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4,
                                        ARG5& arg5){
        (obj->*f)(arg1, arg2, arg3, arg4, arg5);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3, ARG4& arg4){
        (obj->*f)(arg1, arg2, arg3, arg4);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2, typename ARG3>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2, ARG3& arg3){
        (obj->*f)(arg1, arg2, arg3);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1, typename ARG2>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1, ARG2& arg2){
        (obj->*f)(arg1, arg2);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC, typename ARG1>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f, ARG1& arg1){
        (obj->*f)(arg1);

    }
    template<typename FUNC_CLASS_TYPE, typename FUNC>
    static void callMethod(FUNC_CLASS_TYPE* obj, ScriptArgs& args, FUNC f){
        (obj->*f)();
    }
};
template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
struct ScriptFunctorUtil<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>{
    typedef RET (*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                typedef typename RefTypeTraits<ARG6>::RealType RealType6;
                typedef typename RefTypeTraits<ARG7>::RealType RealType7;
                typedef typename RefTypeTraits<ARG8>::RealType RealType8;
                typedef typename RefTypeTraits<ARG9>::RealType RealType9;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();
                RealType6 arg6 = TypeInitValUtil<RealType6>::initVal();
                RealType7 arg7 = TypeInitValUtil<RealType7>::initVal();
                RealType8 arg8 = TypeInitValUtil<RealType8>::initVal();
                RealType9 arg9 = TypeInitValUtil<RealType9>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CppScriptValutil<RealType6>::toCppVal(args.at(5), arg6);
                CppScriptValutil<RealType7>::toCppVal(args.at(6), arg7);
                CppScriptValutil<RealType8>::toCppVal(args.at(7), arg8);
                CppScriptValutil<RealType9>::toCppVal(args.at(8), arg9);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
            }
        }
        DestFunc destFunc;
    };
};
template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5, typename ARG6, typename ARG7, typename ARG8>
struct ScriptFunctorUtil<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>{
    typedef RET (*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                typedef typename RefTypeTraits<ARG6>::RealType RealType6;
                typedef typename RefTypeTraits<ARG7>::RealType RealType7;
                typedef typename RefTypeTraits<ARG8>::RealType RealType8;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();
                RealType6 arg6 = TypeInitValUtil<RealType6>::initVal();
                RealType7 arg7 = TypeInitValUtil<RealType7>::initVal();
                RealType8 arg8 = TypeInitValUtil<RealType8>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CppScriptValutil<RealType6>::toCppVal(args.at(5), arg6);
                CppScriptValutil<RealType7>::toCppVal(args.at(6), arg7);
                CppScriptValutil<RealType8>::toCppVal(args.at(7), arg8);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
            }
        }
        DestFunc destFunc;
    };
};

template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5, typename ARG6, typename ARG7>
struct ScriptFunctorUtil<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>{
    typedef RET (*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                typedef typename RefTypeTraits<ARG6>::RealType RealType6;
                typedef typename RefTypeTraits<ARG7>::RealType RealType7;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();
                RealType6 arg6 = TypeInitValUtil<RealType6>::initVal();
                RealType7 arg7 = TypeInitValUtil<RealType7>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CppScriptValutil<RealType6>::toCppVal(args.at(5), arg6);
                CppScriptValutil<RealType7>::toCppVal(args.at(6), arg7);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            }
        }
        DestFunc destFunc;
    };
};

template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5, typename ARG6>
struct ScriptFunctorUtil<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>{
    typedef RET (*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                typedef typename RefTypeTraits<ARG6>::RealType RealType6;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();
                RealType6 arg6 = TypeInitValUtil<RealType6>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CppScriptValutil<RealType6>::toCppVal(args.at(5), arg6);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1, arg2, arg3, arg4, arg5, arg6);
            }
        }
        DestFunc destFunc;
    };
};

template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5>
struct ScriptFunctorUtil<RET (*)(ARG1, ARG2, ARG3, ARG4, ARG5)>{
    typedef RET (*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1, arg2, arg3, arg4, arg5);
            }
        }
        DestFunc destFunc;
    };
};

template<typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
struct ScriptFunctorUtil<RET (*)(ARG1, ARG2, ARG3, ARG4)>{
    typedef RET (*DestFunc)(ARG1, ARG2, ARG3, ARG4);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1, arg2, arg3, arg4);
            }
        }
        DestFunc destFunc;
    };
};

template<typename RET, typename ARG1, typename ARG2, typename ARG3>
struct ScriptFunctorUtil<RET (*)(ARG1, ARG2, ARG3)>{
    typedef RET (*DestFunc)(ARG1, ARG2, ARG3);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1, arg2, arg3);
            }
        }
        DestFunc destFunc;
    };
};

template<typename RET, typename ARG1, typename ARG2>
struct ScriptFunctorUtil<RET (*)(ARG1, ARG2)>{
    typedef RET (*DestFunc)(ARG1, ARG2);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1, arg2);
            }
        }
        DestFunc destFunc;
    };
};

template<typename RET, typename ARG1>
struct ScriptFunctorUtil<RET (*)(ARG1)>{
    typedef RET (*DestFunc)(ARG1);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CallFuncRetUtil<RET>::call(args, destFunc, arg1);
            }
        }
        DestFunc destFunc;
    };
};

template<typename RET>
struct ScriptFunctorUtil<RET (*)()>{
    typedef RET (*DestFunc)();
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f):destFunc(f){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                CallFuncRetUtil<RET>::call(args, destFunc);
            }
        }
        DestFunc destFunc;
    };
};

template<typename FUNC_CLASS_TYPE>
struct ScriptFunctorUtil<void (FUNC_CLASS_TYPE::*)(ScriptArgs&)>{
    typedef void (FUNC_CLASS_TYPE::*DestFunc)(ScriptArgs&);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                (obj->*(destFunc))(args);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                typedef typename RefTypeTraits<ARG6>::RealType RealType6;
                typedef typename RefTypeTraits<ARG7>::RealType RealType7;
                typedef typename RefTypeTraits<ARG8>::RealType RealType8;
                typedef typename RefTypeTraits<ARG9>::RealType RealType9;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();
                RealType6 arg6 = TypeInitValUtil<RealType6>::initVal();
                RealType7 arg7 = TypeInitValUtil<RealType7>::initVal();
                RealType8 arg8 = TypeInitValUtil<RealType8>::initVal();
                RealType9 arg9 = TypeInitValUtil<RealType9>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CppScriptValutil<RealType6>::toCppVal(args.at(5), arg6);
                CppScriptValutil<RealType7>::toCppVal(args.at(6), arg7);
                CppScriptValutil<RealType8>::toCppVal(args.at(7), arg8);
                CppScriptValutil<RealType9>::toCppVal(args.at(8), arg9);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5, typename ARG6, typename ARG7, typename ARG8>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                typedef typename RefTypeTraits<ARG6>::RealType RealType6;
                typedef typename RefTypeTraits<ARG7>::RealType RealType7;
                typedef typename RefTypeTraits<ARG8>::RealType RealType8;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();
                RealType6 arg6 = TypeInitValUtil<RealType6>::initVal();
                RealType7 arg7 = TypeInitValUtil<RealType7>::initVal();
                RealType8 arg8 = TypeInitValUtil<RealType8>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CppScriptValutil<RealType6>::toCppVal(args.at(5), arg6);
                CppScriptValutil<RealType7>::toCppVal(args.at(6), arg7);
                CppScriptValutil<RealType8>::toCppVal(args.at(7), arg8);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5, typename ARG6, typename ARG7>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                typedef typename RefTypeTraits<ARG6>::RealType RealType6;
                typedef typename RefTypeTraits<ARG7>::RealType RealType7;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();
                RealType6 arg6 = TypeInitValUtil<RealType6>::initVal();
                RealType7 arg7 = TypeInitValUtil<RealType7>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CppScriptValutil<RealType6>::toCppVal(args.at(5), arg6);
                CppScriptValutil<RealType7>::toCppVal(args.at(6), arg7);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5, typename ARG6>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                typedef typename RefTypeTraits<ARG6>::RealType RealType6;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();
                RealType6 arg6 = TypeInitValUtil<RealType6>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CppScriptValutil<RealType6>::toCppVal(args.at(5), arg6);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1, arg2, arg3, arg4, arg5, arg6);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4,
        typename ARG5>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4, ARG5)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1, ARG2, ARG3, ARG4, ARG5);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                typedef typename RefTypeTraits<ARG5>::RealType RealType5;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();
                RealType5 arg5 = TypeInitValUtil<RealType5>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CppScriptValutil<RealType5>::toCppVal(args.at(4), arg5);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1, arg2, arg3, arg4, arg5);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1, ARG2, ARG3, ARG4)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1, ARG2, ARG3, ARG4);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                typedef typename RefTypeTraits<ARG4>::RealType RealType4;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();
                RealType4 arg4 = TypeInitValUtil<RealType4>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CppScriptValutil<RealType4>::toCppVal(args.at(3), arg4);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1, arg2, arg3, arg4);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1, typename ARG2, typename ARG3>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1, ARG2, ARG3)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1, ARG2, ARG3);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                typedef typename RefTypeTraits<ARG3>::RealType RealType3;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();
                RealType3 arg3 = TypeInitValUtil<RealType3>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CppScriptValutil<RealType3>::toCppVal(args.at(2), arg3);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1, arg2, arg3);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1, typename ARG2>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1, ARG2)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1, ARG2);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                typedef typename RefTypeTraits<ARG2>::RealType RealType2;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();
                RealType2 arg2 = TypeInitValUtil<RealType2>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CppScriptValutil<RealType2>::toCppVal(args.at(1), arg2);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1, arg2);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};

template<typename FUNC_CLASS_TYPE, typename RET, typename ARG1>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)(ARG1)>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)(ARG1);
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                typedef typename RefTypeTraits<ARG1>::RealType RealType1;
                RealType1 arg1 = TypeInitValUtil<RealType1>::initVal();

                CppScriptValutil<RealType1>::toCppVal(args.at(0), arg1);
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc, arg1);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};
template<typename FUNC_CLASS_TYPE, typename RET>
struct ScriptFunctorUtil<RET (FUNC_CLASS_TYPE::*)()>{
    typedef RET (FUNC_CLASS_TYPE::*DestFunc)();
    class ScriptFunctorImpl: public ScriptFunctor{
    public:
        ScriptFunctorImpl(DestFunc f, FUNC_CLASS_TYPE* p):destFunc(f), obj(p){}
        virtual void callFunc(ScriptArgs& args){
            if (destFunc){
                CallFuncRetUtil<RET>::callMethod(obj, args, destFunc);
            }
        }
        DestFunc destFunc;
        FUNC_CLASS_TYPE* obj;
    };
};
}
#endif

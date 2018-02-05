#ifndef _FF_PROP_H_
#define _FF_PROP_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include "base/fftype.h"
#include "base/singleton.h"


namespace ff
{
template<typename T> struct EntityFieldTool;

template<typename BASE_TYPE, typename ObjType, typename RET, typename CLASS_TYPE>
class FieldPropGetterSertter: public BASE_TYPE{
public:
    RET CLASS_TYPE::* ptr;
    FieldPropGetterSertter(RET CLASS_TYPE::* p):ptr(p){}
    virtual int64_t getProp(ObjType obj){
        CLASS_TYPE* pField = EntityFieldTool<CLASS_TYPE>::getField(obj);
        return (int64_t)(pField->*ptr);
        return 0;
    }
    virtual void setProp(ObjType obj, int64_t v){
        CLASS_TYPE* pField = EntityFieldTool<CLASS_TYPE>::getField(obj);
        pField->*ptr = (RET)v;
    }
};
//!各个属性对应一个总值 
//!各个属性对应各个模块的分值
template<typename T>
class PropCommonMgr
{
public:
    typedef T ObjType;
    typedef int64_t (*functorGet)(ObjType);
    typedef void (*functorSet)(ObjType, int64_t);
    class PropGetterSetter
    {
        public:
        virtual ~PropGetterSetter(){}
        PropGetterSetter():fGetter(NULL), fSetter(NULL){}        
        functorGet fGetter;
        functorSet fSetter;
        std::map<std::string, int64_t> moduleRecord;

        virtual int64_t getProp(ObjType obj){
            if (fGetter)
                return fGetter(obj);
            return 0;
        }
        virtual void setProp(ObjType obj, int64_t v){
            if (fSetter)
                fSetter(obj, v);
        }
    };
    ~PropCommonMgr(){
        typename std::map<std::string, PropGetterSetter*>::iterator it = propName2GetterSetter.begin();
        for (; it != propName2GetterSetter.end(); ++it){
            delete it->second;
            it->second = NULL;
        }
        propName2GetterSetter.clear();
    }
    int64_t get(ObjType obj, const std::string& strName) {
        typename std::map<std::string, PropGetterSetter*>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end()){
            return it->second->getProp(obj);
        }
        return 0;
    }
    bool set(ObjType obj, const std::string& strName, int64_t v) {
        typename std::map<std::string, PropGetterSetter*>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end()){
            it->second->setProp(obj, v);
            return true;
        }
        return false;
    }
    int64_t addByModule(ObjType obj, const std::string& strName, const std::string& moduleName, int64_t v) {
        typename std::map<std::string, PropGetterSetter*>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end()){
            int64_t ret =it->second->getProp(obj);
            std::map<std::string, int64_t>::iterator itMod = it->second.moduleRecord.find(moduleName);
            if (itMod != it->second.moduleRecord.end()){
                ret -= itMod->second;
                itMod->second = v;
            }
            else{
                it->second.moduleRecord[moduleName] = v;
            }
            ret += v;
            it->second->setProp(obj, ret);
            return ret;
        }
        return 0;
    }
    int64_t subByModule(ObjType obj, const std::string& strName, const std::string& moduleName) {
        typename std::map<std::string, PropGetterSetter*>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end()){
            int64_t ret =it->second->getProp(obj);
            std::map<std::string, int64_t>::iterator itMod = it->second.moduleRecord.find(moduleName);
            if (itMod == it->second.moduleRecord.end()){
                return ret;
            }
            ret -= itMod->second;
            it->second.moduleRecord.erase(itMod);
            it->second->setProp(obj, ret);
            return ret;
        }
        return 0;
    }
    int64_t getByModule(ObjType obj, const std::string& strName, const std::string& moduleName) {
        typename std::map<std::string, PropGetterSetter*>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end() && it->second.fGet && it->second.fSet){
            int64_t ret =it->second.fGet(obj);
            std::map<std::string, int64_t>::iterator itMod = it->second.moduleRecord.find(moduleName);
            if (itMod != it->second.moduleRecord.end()){
                return itMod->second;
            }
        }
        return 0;
    }
    std::map<std::string, int64_t> getAllModule(ObjType obj, const std::string& strName) {
        std::map<std::string, int64_t> ret;
        typename std::map<std::string, PropGetterSetter*>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end() && it->second.fGet && it->second.fSet){
            ret = it->second.moduleRecord;
        }
        return ret;
    }
    void regGetterSetter(const std::string& strName, functorGet fGet, functorSet fSet){
        PropGetterSetter* info = new PropGetterSetter();
        info->fGetter = fGet;
        info->fSetter = fSet;
        propName2GetterSetter[strName] = info;
    }
    void regGetterSetter(const std::string& strName, PropGetterSetter* p){
        propName2GetterSetter[strName] = p;
    }
    template<typename RET, typename CLASS_TYPE>
    void regGetterSetter(const std::string& strName, RET CLASS_TYPE::* p_){
        FieldPropGetterSertter<PropGetterSetter, ObjType, RET, CLASS_TYPE>* pdest = new FieldPropGetterSertter<PropGetterSetter, ObjType, RET, CLASS_TYPE>(p_);
        regGetterSetter(strName, pdest);
    }
public:
    std::map<std::string, PropGetterSetter*>    propName2GetterSetter;
};


}
#endif

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


//!各个属性对应一个总值 
//!各个属性对应各个模块的分值
template<typename T>
class PropCommonMgr
{
public:
    typedef T ObjType;
    typedef int64_t (*functorGet)(ObjType);
    typedef void (*functorSet)(ObjType, int64_t);
    struct PropGetterSetter
    {
        PropGetterSetter():fGet(NULL), fSet(NULL){}        
        functorGet fGet;
        functorSet fSet;
        std::map<std::string, int64_t> moduleRecord;
    };

    int64_t get(ObjType obj, const std::string& strName) {
        typename std::map<std::string, PropGetterSetter>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end() && it->second.fGet){
            return it->second.fGet(obj);
        }
        return 0;
    }
    bool set(ObjType obj, const std::string& strName, int64_t v) {
        typename std::map<std::string, PropGetterSetter>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end() && it->second.fSet){
            it->second.fSet(obj, v);
            return true;
        }
        return false;
    }
    int64_t addByModule(ObjType obj, const std::string& strName, const std::string& moduleName, int64_t v) {
        typename std::map<std::string, PropGetterSetter>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end() && it->second.fGet && it->second.fSet){
            int64_t ret =it->second.fGet(obj);
            std::map<std::string, int64_t>::iterator itMod = it->second.moduleRecord.find(moduleName);
            if (itMod != it->second.moduleRecord.end()){
                ret -= itMod->second;
                itMod->second = v;
            }
            else{
                it->second.moduleRecord[moduleName] = v;
            }
            ret += v;
            it->second.fSet(obj, ret);
            return ret;
        }
        return 0;
    }
    int64_t subByModule(ObjType obj, const std::string& strName, const std::string& moduleName) {
        typename std::map<std::string, PropGetterSetter>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end() && it->second.fGet && it->second.fSet){
            int64_t ret =it->second.fGet(obj);
            std::map<std::string, int64_t>::iterator itMod = it->second.moduleRecord.find(moduleName);
            if (itMod == it->second.moduleRecord.end()){
                return ret;
            }
            ret -= itMod->second;
            it->second.moduleRecord.erase(itMod);
            it->second.fSet(obj, ret);
            return ret;
        }
        return 0;
    }
    int64_t getByModule(ObjType obj, const std::string& strName, const std::string& moduleName) {
        typename std::map<std::string, PropGetterSetter>::iterator it = propName2GetterSetter.find(strName);
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
        typename std::map<std::string, PropGetterSetter>::iterator it = propName2GetterSetter.find(strName);
        if (it != propName2GetterSetter.end() && it->second.fGet && it->second.fSet){
            ret = it->second.moduleRecord;
        }
        return ret;
    }
    void regGetterSetter(const std::string& strName, functorGet fGet, functorSet fSet){
        PropGetterSetter info;
        info.fGet = fGet;
        info.fSet = fSet;
        propName2GetterSetter[strName] = info;
    }
public:
    std::map<std::string, PropGetterSetter>    propName2GetterSetter;
};

}
#endif

#ifndef _FF_SCRIPT_CACHE_H_
#define _FF_SCRIPT_CACHE_H_

#include <string>
#include <vector>
#include <map>

#include "base/singleton.h"
#include "base/strtool.h"
#include "server/script.h"

namespace ff
{
#define SCRIPT_CACHE Singleton<ScriptCache>::instance()
class ScriptCache{
public:
    ScriptCache()
    {
        m_nullData  = new ScriptArgObj();
        m_dataCache = new ScriptArgObj();
        m_dataCache->toDict();
    }
    ScriptArgObjPtr get(const std::string& key)
    {
        if (key.empty()){
            return m_dataCache;
        }
        std::vector<std::string> keyArgs;
        StrTool::split(key, keyArgs, ".");
        
        ScriptArgObjPtr curData = m_dataCache;
        for (size_t i = 0; i < keyArgs.size(); ++i){
            const std::string& curKey = keyArgs[i];
            if (curKey.empty()){
                return m_nullData;
            }
            if (false == curData->isDict()){
                return m_nullData;
            }
            
            const std::map<std::string, ScriptArgObjPtr>& dataDict = curData->getDict();
            if (curKey[curKey.size() - 1] != ']')//!dict get 
            {
                std::map<std::string, ScriptArgObjPtr>::const_iterator it = dataDict.find(curKey);
                if (it != dataDict.end()){
                    curData = it->second;
                }
                else{
                    return m_nullData;
                }
            }
            else{
                std::vector<std::string> indexArgs;
                StrTool::split(curKey, indexArgs, "[");
                if (indexArgs.size() != 2){
                    return m_nullData;
                }
                std::map<std::string, ScriptArgObjPtr>::const_iterator it = dataDict.find(indexArgs[0]);
                if (it != dataDict.end()){
                    curData = it->second;
                }
                else{
                    return m_nullData;
                }
                
                if (false == curData->isList()){
                    return m_nullData;
                }
                
                int nIndex = ::atoi(indexArgs[1].c_str());
                if (nIndex < 0 || nIndex >= curData->getList().size()){
                    return m_nullData;
                }
                curData = curData->getList()[nIndex];
            }
        }
        return curData;
    }
    bool set(const std::string& key, ScriptArgObjPtr v){
        if (key.empty()){
            return false;
        }
        std::vector<std::string> keyArgs;
        StrTool::split(key, keyArgs, ".");
        
        ScriptArgObjPtr* curData = &m_dataCache;
        size_t i = 0;
        for (; i < keyArgs.size(); ++i){
            const std::string& curKey = keyArgs[i];
            if (curKey.empty()){
                return false;
            }
            
            if (false == (*curData)->isDict()){
                (*curData)->toDict();
            }

            std::map<std::string, ScriptArgObjPtr>& dataDict = (*curData)->dictVal;
            if (curKey[curKey.size() - 1] != ']')//!dict set 
            {
                
                curData = &dataDict[curKey];
                if (!(*curData)){
                    (*curData) = new ScriptArgObj();
                }
            }
            else{
                std::vector<std::string> indexArgs;
                StrTool::split(curKey, indexArgs, "[");
                if (indexArgs.size() != 2){
                    return m_nullData;
                }
                curData = &dataDict[indexArgs[0]];
                if (!(*curData)){
                    (*curData) = new ScriptArgObj();
                }
                if (false == (*curData)->isList()){
                    (*curData)->toList();
                }
                
                int nIndex = ::atoi(indexArgs[1].c_str());
                if (nIndex < 0){
                    return false;
                }
                for (int i = (int)(*curData)->getList().size(); i <= nIndex; ++i){
                    ScriptArgObjPtr p = new ScriptArgObj();
                    (*curData)->listVal.push_back(p);
                }
                curData = &((*curData)->listVal[nIndex]);
            }
        }
        
        (*curData) = v;
        
        return true;
    }
    bool del(const std::string& key){
        if (key.empty()){
            return false;
        }
        std::vector<std::string> keyArgs;
        StrTool::split(key, keyArgs, ".");
        
        ScriptArgObjPtr* curData = &m_dataCache;
        size_t i = 0;
        for (; i < keyArgs.size(); ++i){
            const std::string& curKey = keyArgs[i];
            if (curKey.empty()){
                return false;
            }
            
            if (false == (*curData)->isDict()){
                (*curData)->toDict();
            }

            std::map<std::string, ScriptArgObjPtr>& dataDict = (*curData)->dictVal;
            if (curKey[curKey.size() - 1] != ']')//!dict set 
            {
                if (i == keyArgs.size() -1){
                    dataDict.erase(curKey);
                    return true;
                }
                curData = &dataDict[curKey];
                if (!(*curData)){
                    (*curData) = new ScriptArgObj();
                }
            }
            else{
                std::vector<std::string> indexArgs;
                StrTool::split(curKey, indexArgs, "[");
                if (indexArgs.size() != 2){
                    return m_nullData;
                }
                if (dataDict.find(indexArgs[0]) == dataDict.end()){
                    return false;
                }
                curData = &dataDict[indexArgs[0]];
                if (!(*curData)){
                    return false;
                }
                if (false == (*curData)->isList()){
                    return false;
                }
                
                int nIndex = ::atoi(indexArgs[1].c_str());
                if (nIndex < 0 || nIndex >= (int)(*curData)->getList().size()){
                    return false;
                }
                if (i == keyArgs.size() -1){
                    (*curData)->listVal.erase((*curData)->listVal.begin() + nIndex);
                    return true;
                }
                
                curData = &((*curData)->listVal[nIndex]);
            }
        }
        
        return true;
    }
    size_t size(const std::string& key)
    {
        ScriptArgObjPtr ret = this->get(key);
        if (ret->isList()){
            return ret->getList().size();
        }
        else if (ret->isDict()){
            return ret->getDict().size();
        }
        else if (ret->isString()){
            return ret->getString().size();
        }
        return 0;
    }
    
    bool init(){
        SCRIPT_UTIL.reg("Cache.get", &ScriptCache::get, this);
        SCRIPT_UTIL.reg("Cache.set", &ScriptCache::set, this);
        SCRIPT_UTIL.reg("Cache.del", &ScriptCache::del, this);
        SCRIPT_UTIL.reg("Cache.size", &ScriptCache::size, this);
        return true;
    }
    
public:
    ScriptArgObjPtr     m_nullData;
    ScriptArgObjPtr     m_dataCache;
};

}
#endif

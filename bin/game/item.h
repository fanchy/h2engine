#ifndef _FF_ITEM_H_
#define _FF_ITEM_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "game/entity.h"
#include "base/smart_ptr.h"

namespace ff
{

struct ItemConfig{
    ItemConfig():cfgid(0){}
    int64_t getPropNum(const std::string& key);
    std::string getPropStr(const std::string& key);

    int             cfgid;
    std::string     name;

    std::map<std::string, int64_t>      numProp;
    std::map<std::string, std::string>  strProp;
};
typedef SharedPtr<ItemConfig> ItemConfigPtr;

class ItemMgr
{
public:
    ItemConfigPtr getCfg(int cfgid) { return m_itemCfgs[cfgid]; }

    bool init();
public:
    std::map<int, ItemConfigPtr>    m_itemCfgs;
};

class ItemCtrl:public EntityField
{
public:
    virtual void onEvent(EntityPtr& entity, EntityEvent& event){}//!处理事件通知

    ItemConfigPtr getCfg()          { return m_itemConfig; }
    void setCfg(ItemConfigPtr c)    { m_itemConfig = c;    }
    
    int getPosition() const         { return m_nPosition;  }
    void setPosition(int n)         { m_nPosition = n;     }
    
    int getNum() const              { return m_nNum;       }
    void setNum(int n)              { m_nNum = n ;         }
    
    int getCfgId()                  {return m_itemConfig->cfgid; }
    const std::string& getCfgName() {return m_itemConfig->name;  }
    
    EntityPtr getItemOwner()        {return m_refItemOwner.lock();}
    void setItemOwner(EntityPtr e)  {if (e) m_refItemOwner = e; else m_refItemOwner.reset();}
public:
    EntityRef           m_refItemOwner;
    int                 m_nPosition;    //!在包裹中的位置
    int                 m_nNum;         //!叠加数量
    ItemConfigPtr       m_itemConfig;
};

class PkgCtrl:public EntityField
{
public:
    PkgCtrl():m_nMaxNum(0){}
    ~PkgCtrl();
    virtual void onEvent(EntityPtr& entity, EntityEvent& event){}//!处理事件通知

    void addItem(EntityPtr item);
    EntityPtr genItem(uint64_t uid, int cfgid);
    bool delItem(uint64_t uid);
    void clear() { m_allItems.clear(); }
    
    EntityPtr getItem(uint64_t uid);
    int       getItem(int cfgid, std::vector<EntityPtr>& ret);
    EntityPtr getItem(const std::string& name);
    int       getItem(const std::string& name, std::vector<EntityPtr>& ret);
    
    EntityPtr getItemByProp(const std::string& key, int64_t propVal);
    EntityPtr getItemByProp(const std::string& key, const std::string& propVal);
    int       getItemByProp(const std::string& key, int propVal, std::vector<EntityPtr>& ret);
    int       getItemByProp(const std::string& key, const std::string& propVal, std::vector<EntityPtr>& ret);
    
    EntityPtr getItemByPosition(int pos);
    
    int curSize() const     { return m_allItems.size(); }
    int maxNum()  const     { return m_nMaxNum;         }
    void setMaxNum(int n)   { m_nMaxNum  = n;           }
    int leftNum() const     { return m_nMaxNum - m_allItems.size();}
    
    template<typename T>
    void foreach(T func){
        for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
            func(it->second);
        }
    }
public:
    int                             m_nMaxNum;
    std::map<uint64_t, EntityPtr>   m_allItems;
};

}

#endif

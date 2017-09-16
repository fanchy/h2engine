#include <ctype.h>

#include "item/item.h"
#include "server/db_mgr.h"
#include "common/game_event.h"

using namespace ff;
using namespace std;

int64_t ItemConfig::getPropNum(const std::string& key){
    std::map<std::string, int64_t>::iterator it = numProp.find(key);
    if (it != numProp.end()){
        return it->second;
    }
    return 0;
}

static bool isNumberStr(const string& s){
    if (s.empty()){
        return false;
    }
    for (size_t i = 0; i < s.size(); ++i){
        if (!::isdigit(s[i])){
            return false;
        }
    }
    return false;
}

string ItemConfig::getPropStr(const std::string& key){
    std::map<std::string, std::string>::iterator it = strProp.find(key);
    if (it != strProp.end()){
        return it->second;
    }
    return "";
}
bool ItemMgr::init(){
    vector<vector<string> > retdata; 
    vector<string> col;
    string errinfo;
    int affectedRows = 0;
    
    string sql = "select * from itemcfg";
    DB_MGR_OBJ.syncQueryDBGroupMod("cfgDB", 0, sql, retdata, col, errinfo, affectedRows);
    
    if (retdata.empty()){
        return false;
    }
    
    m_itemCfgs.clear();
    for (size_t i = 0; i < retdata.size(); ++i){
        vector<string>& row = retdata[i];
        ItemConfigPtr cfgItem = new ItemConfig();
        
        for (size_t j = 0; j < row.size(); ++j){
            const string& keyname = col[j];
            if (isNumberStr(row[j])){
                cfgItem->numProp[keyname] = ::atol(row[j].c_str());
            }
            else{
                cfgItem->strProp[keyname] = row[j];
            }
        }
        
        cfgItem->cfgid = int(cfgItem->getPropNum("cfgid"));
        cfgItem->name  = cfgItem->getPropStr("name");
        m_itemCfgs[cfgItem->cfgid] = cfgItem;
    }
    return true;
}

EntityPtr PkgCtrl::getItem(uint64_t uid){
    std::map<uint64_t, EntityPtr>::iterator it = m_allItems.find(uid);
    if (it != m_allItems.end()){
        return it->second;
    }
    return NULL;
}

int PkgCtrl::getItem(int cfgid, std::vector<EntityPtr>& ret){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        if (it->second->get<ItemCtrl>()->getCfg()->cfgid == cfgid){
            ret.push_back(it->second); 
        }
    }
    return int(ret.size());
}

EntityPtr PkgCtrl::getItem(const string& name){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        if (it->second->get<ItemCtrl>()->getCfg()->name == name){
            return it->second; 
        }
    }
    return NULL;
}
int PkgCtrl::getItem(const string& name, std::vector<EntityPtr>& ret){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        if (it->second->get<ItemCtrl>()->getCfg()->name == name){
            ret.push_back(it->second); 
        }
    }
    return int(ret.size());
}

EntityPtr PkgCtrl::getItemByProp(const std::string& key, int64_t propVal){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        if (it->second->get<ItemCtrl>()->getCfg()->getPropNum(key) == propVal){
            return it->second; 
        }
    }
    return NULL;
}
EntityPtr PkgCtrl::getItemByProp(const std::string& key, const std::string& propVal){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        if (it->second->get<ItemCtrl>()->getCfg()->getPropStr(key) == propVal){
            return it->second; 
        }
    }
    return NULL;
}
int PkgCtrl::getItemByProp(const std::string& key, int propVal, std::vector<EntityPtr>& ret){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        if (it->second->get<ItemCtrl>()->getCfg()->getPropNum(key) == propVal){
            ret.push_back(it->second); 
        }
    }
    return int(ret.size());
}
int PkgCtrl::getItemByProp(const std::string& key, const std::string& propVal, std::vector<EntityPtr>& ret){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        if (it->second->get<ItemCtrl>()->getCfg()->getPropStr(key) == propVal){
            ret.push_back(it->second); 
        }
    }
    return int(ret.size());
}

EntityPtr PkgCtrl::getItemByPosition(int pos){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        if (it->second->get<ItemCtrl>()->getPosition() == pos){
            return it->second; 
        }
    }
    return NULL;
}

void PkgCtrl::addItem(EntityPtr item){
    m_allItems[item->getUid()] = item;

    CreateEntityEvent eItemAddEvent(item);
    EVENT_BUS_FIRE(eItemAddEvent);
}

EntityPtr PkgCtrl::genItem(uint64_t uid, int cfgid){
    ItemConfigPtr cfg = Singleton<ItemMgr>::instance().getCfg(cfgid);
    if (!cfg){
        return NULL;
    }
    
    EntityPtr item = Entity::genEntity(ENTITY_ITEM, uid);
    item->get<ItemCtrl>()->setCfg(cfg);
    item->get<ItemCtrl>()->setItemOwner(this->getOwner());
    this->addItem(item);
    
    return item;
}

bool PkgCtrl::delItem(uint64_t uid){
    std::map<uint64_t, EntityPtr>::iterator it = m_allItems.find(uid);
    if (it != m_allItems.end()){
        EntityPtr item = it->second;
        m_allItems.erase(it);

        DelEntityEvent eItemDelEvent(item);
        EVENT_BUS_FIRE(eItemDelEvent);

        return true;
    }
    return false;
}
PkgCtrl::~PkgCtrl(){
    for (std::map<uint64_t, EntityPtr>::iterator it = m_allItems.begin(); it != m_allItems.end(); ++it){
        EntityPtr& item = it->second;
        DELETE_ENTITY(item);
    }
    m_allItems.clear();
}

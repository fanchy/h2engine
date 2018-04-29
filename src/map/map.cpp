
#include<stdio.h>
#include<math.h>

#include "base/log.h"
#include "server/ffworker.h"
#include "map/map.h"
#include "common/game_event.h"
#include "base/os_tool.h"

using namespace ff;
using namespace std;

int64_t MapConfig::getPropNum(const std::string& key){
    string v = getPropStr(key);
    if (v.empty() == false){
        return ::atol(v.c_str());
    }
    return 0;
}

string MapConfig::getPropStr(const std::string& key){
    string keyname = key;
    std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::toupper);
    std::map<std::string, std::string>::iterator it = allProp.find(keyname);
    if (it != allProp.end()){
        return it->second;
    }
    return "";
}

void MapPoint::getAllSession(std::vector<userid_t>& ret){
    map<userid_t, EntityRef>::iterator it2 = this->entities.begin();
    for (; it2 != this->entities.end(); ++it2){
        EntityPtr eachEntity = it2->second.lock();
        if (eachEntity){
            ret.push_back(eachEntity->getSession());
        }
    }
}
void MapPoint::getAllEntity(std::vector<EntityPtr>& ret, userid_t uidIgnore){
    map<userid_t, EntityRef>::iterator it2 = this->entities.begin();
    for (; it2 != this->entities.end(); ++it2){
        EntityPtr eachEntity = it2->second.lock();
        if (eachEntity && eachEntity->getUid() != uidIgnore){
            ret.push_back(eachEntity);
        }
    }
}

void Map9Grid::getAllSession(std::vector<userid_t>& ret){
    map<userid_t, EntityRef>::iterator it2 = this->entities.begin();
    for (; it2 != this->entities.end(); ++it2){
        EntityPtr eachEntity = it2->second.lock();
        if (eachEntity){
            ret.push_back(eachEntity->getSession());
        }
    }
}
void Map9Grid::getAllEntity(std::vector<EntityPtr>& ret, userid_t uidIgnore){
    map<userid_t, EntityRef>::iterator it2 = this->entities.begin();
    for (; it2 != this->entities.end(); ++it2){
        EntityPtr eachEntity = it2->second.lock();
        if (eachEntity && eachEntity->getUid() != uidIgnore){
            ret.push_back(eachEntity);
        }
    }
}

MapObj::MapObj(std::string s, MapConfigPtr mapCfg):mapId(s), width(mapCfg->width), height(mapCfg->height)
{
    m_allPos.resize(width * height);
    int nEachGridSize = GRID_SIZE;
    width9Grid  = int(::ceil(width * 1.0 / nEachGridSize));
    height9Grid = int(::ceil(height * 1.0 / nEachGridSize));
    
    LOGTRACE(("XX", "MapObj mapname:%s,w:%d,h:%d,gridw:%d,gridh:%d,nEachGridSize:%d", mapId, width, height, width9Grid, height9Grid, nEachGridSize));
}

std::vector<EntityPtr> MapObj::rangeGetEntities(int x, int y, int radius, bool includeCenter){
    vector<EntityPtr> ret;
    for (int i = 1; i <= radius; ++i){
        int yoffset = y + i;
        for (int j = 1; j <= radius; ++j){
            int xoffset = x + j;
            MapPoint* point = getPoint(xoffset, yoffset);
            if (point){
                map<userid_t, EntityRef>::iterator it = point->entities.begin();
                for (; it != point->entities.end(); ++it){
                    EntityPtr p = it->second.lock();
                    if (p)
                        ret.push_back(p);
                }
            }
        }
    }
    if (includeCenter){
        MapPoint* point = getPoint(x, y);
        if (point){
            map<userid_t, EntityRef>::iterator it = point->entities.begin();
            for (; it != point->entities.end(); ++it){
                EntityPtr p = it->second.lock();
                if (p)
                    ret.push_back(p);
            }
        }
    }
    return ret;
}

MapPoint* MapObj::getPoint(int x, int y){
    int index = y * width + x;
    if (index >= 0 && index < (int)m_allPos.size()){
        return &m_allPos[index];
    }
    return NULL;
}

Map9Grid* MapObj::getGrid(int x, int y, std::set<Map9Grid*>* retSet){
    int nGridX = int(x / width9Grid);
    int nGridY = int(y / height9Grid);

    int index = nGridY * width9Grid + nGridX;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        if (retSet)
            (*retSet).insert(&m_all9Grid[index]);
        return &m_all9Grid[index];
    }
    return NULL;
}

void MapObj::get9Grid(int x, int y, std::set<Map9Grid*>& retSet)
{
    int nGridX = int(x / width9Grid);
    int nGridY = int(y / height9Grid);

    int indexCenter = nGridY * width9Grid + nGridX;
    int index = indexCenter;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
    index = indexCenter - 1;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
    index = indexCenter + 1;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
    
    index = indexCenter - width9Grid;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
    index = indexCenter - width9Grid - 1;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
    index = indexCenter - width9Grid + 1;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
    
    index = indexCenter + width9Grid;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
    index = indexCenter + width9Grid - 1;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
    index = indexCenter + width9Grid + 1;
    if (index >= 0 && index < (int)m_all9Grid.size()){
        retSet.insert(&m_all9Grid[index]);
    }
}
bool MapObj::getEntityPos(userid_t uid, int& x, int& y){
    std::map<userid_t, EntityPos>::iterator it = m_allEntity.find(uid);
    if (it != m_allEntity.end()){
        x = it->second.x;
        y = it->second.y;
        return true;
    }
    return false;
}
void MapObj::updateEntityPos(EntityPtr& entity, int x, int y){
    EntityPos& pos = m_allEntity[entity->getUid()];
    pos.x = x;
    pos.y = y;
    pos.entity = entity;
    entity->get<MapCtrl>()->x = x;
    entity->get<MapCtrl>()->y = y;
}
bool MapObj::getAllEntity(std::vector<EntityPtr>& ret, int nType){
    for (std::map<userid_t, EntityPos>::iterator it = m_allEntity.begin(); it != m_allEntity.end(); ++it){
        EntityPtr entity = it->second.entity.lock();
        
        if (entity && (nType == -1 || entity->getType() == nType)){
            ret.push_back(entity);
        }
    }
    return true;
}
bool MapObj::enter(EntityPtr& entity, int x, int y){
    
    MapPoint* mp = getPoint(x, y);
    Map9Grid* mg = getGrid(x, y);
    
    if (NULL == mp || NULL == mg){
        return false;
    }
    EnterMapCheckEvent eEnterMapCheckEvent(entity, *this, x, y);
    EVENT_BUS_FIRE(eEnterMapCheckEvent);
    if (eEnterMapCheckEvent.isOk == false){
        return false;
    }
    
    if (entity->get<MapCtrl>()->curMap){
        entity->get<MapCtrl>()->curMap->leave(entity);
    }
    
    mp->entities[entity->getUid()] = entity;
    mg->entities[entity->getUid()] = entity;
    this->updateEntityPos(entity, x, y);
    
    EnterMapEvent eEnterMapEvent(entity);
    
    std::set<Map9Grid*> retSet;
    get9Grid(x, y, retSet);
    
    for (set<Map9Grid*>::iterator it = retSet.begin(); it != retSet.end(); ++it){
        Map9Grid* eachmg = *it;
        map<userid_t, EntityRef>::iterator it2 = eachmg->entities.begin();
        for (; it2 != eachmg->entities.end(); ++it2){
            EntityPtr eachEntity = it2->second.lock();
            if (eachEntity == entity){
                continue;
            }
            else if (eachEntity){
                eEnterMapEvent.AOIEnter.push_back(eachEntity);
            }
        }
    }
    
    EVENT_BUS_FIRE(eEnterMapEvent);
    return true;
}

bool MapObj::leave(EntityPtr& entity){
    int x = 0, y = 0;
    this->getEntityPos(entity->getUid(), x, y);
    
    MapPoint* mp = getPoint(x, y);
    Map9Grid* mg = getGrid(x, y);
    
    if (NULL == mp || NULL == mg){
        return false;
    }
    mp->entities.erase(entity->getUid());
    mg->entities.erase(entity->getUid());
    m_allEntity.erase(entity->getUid());
    
    LeaveMapEvent eLeaveMapEvent(entity);
    
    std::set<Map9Grid*> retSet;
    get9Grid(x, y, retSet);
    
    for (set<Map9Grid*>::iterator it = retSet.begin(); it != retSet.end(); ++it){
        Map9Grid* eachmg = *it;
        map<userid_t, EntityRef>::iterator it2 = eachmg->entities.begin();
        for (; it2 != eachmg->entities.end(); ++it2){
            EntityPtr eachEntity = it2->second.lock();
            if (eachEntity && eachEntity != entity)
            {
                eLeaveMapEvent.AOILeave.push_back(eachEntity);
            }
        }
    }
    EVENT_BUS_FIRE(eLeaveMapEvent);
    return true;
}

bool MapObj::move(EntityPtr& entity, int xTo, int yTo)
{
    int x = 0, y = 0;
    this->getEntityPos(entity->getUid(), x, y);
    
    MapPoint* mp = getPoint(x, y);
    Map9Grid* mg = getGrid(x, y);
    if (NULL == mp || NULL == mg){
        return false;
    }
    
    MapPoint* mpTo = getPoint(xTo, yTo);
    Map9Grid* mgTo = getGrid(xTo, yTo);
    
    if (NULL == mpTo || NULL == mgTo){
        return false;
    }
    MoveMapCheckEvent eMoveMapCheckEvent(entity, xTo, yTo);
    EVENT_BUS_FIRE(eMoveMapCheckEvent);
    if (!eMoveMapCheckEvent.isOk){
        return false;
    }
    
    std::set<Map9Grid*> retSet;
    get9Grid(x, y, retSet);
    
    this->updateEntityPos(entity, xTo, yTo);
    MoveMapEvent eMoveMapEvent(entity, x, y);
    
    if (mg == mgTo){
        mp->entities.erase(entity->getUid());
        mpTo->entities[entity->getUid()] = entity;
        
        mg->getAllEntity(eMoveMapEvent.AOIMove, entity->getUid());
    }
    else{
        mp->entities.erase(entity->getUid());
        mg->entities.erase(entity->getUid());
        
        std::set<Map9Grid*> retSetTo;
        get9Grid(xTo, yTo, retSet);
        
        mpTo->entities[entity->getUid()] = entity;
        mgTo->entities[entity->getUid()] = entity;
        
        for (set<Map9Grid*>::iterator it = retSet.begin(); it != retSet.end(); ++it){
            
            Map9Grid* eachmg = *it;
            set<Map9Grid*>::iterator itTo = retSetTo.find(eachmg);

            if (itTo != retSetTo.end()){//!move msg
                (*itTo)->getAllEntity(eMoveMapEvent.AOIMove, entity->getUid());
            }
            else{
                (*itTo)->getAllEntity(eMoveMapEvent.AOILeave, entity->getUid());
            }
            retSetTo.erase(itTo);
        }
        for (set<Map9Grid*>::iterator it = retSetTo.begin(); it != retSetTo.end(); ++it){
            (*it)->getAllEntity(eMoveMapEvent.AOIEnter, entity->getUid());
        }
    }
    
    EVENT_BUS_FIRE(eMoveMapEvent);
    return true;
}

bool MapMgr::init(){
    vector<vector<string> > retdata;
    string csvData;
    OSTool::readFile("config/map_config.csv", csvData);
    StrTool::loadCsvFromString(csvData, retdata);

    if (retdata.empty()){
        LOGWARN((GAME_LOG, "MapMgr::init load none data"));
        return true;
    }
    vector<string>& col = retdata[0];
    for (size_t i = 1; i < retdata.size(); ++i){
        vector<string>& row = retdata[i];
        MapConfigPtr mapCfg = new MapConfig();

        for (size_t j = 0; j < row.size(); ++j){
            string& keyname = col[j];
            std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::toupper);
            mapCfg->allProp[keyname] = row[j];
        }
        mapCfg->cfgName   = mapCfg->getPropStr("cfgName");
        mapCfg->width     = (int)mapCfg->getPropNum("width");
        mapCfg->height    = (int)mapCfg->getPropNum("height");
        m_allMapCfg[mapCfg->cfgName] = mapCfg;
    }
    return true;
}
bool MapMgr::cleanup(){
    return true;
}

MapObjPtr MapMgr::allocMap(const std::string& cfgName, std::string mapId){
    MapConfigPtr mapCfg = getMapCfg(cfgName);
    if (!mapCfg){
        return NULL;
    }
    if (mapId.empty()){
        char tmpMapId[256] = {0};
        ::snprintf(tmpMapId, sizeof(tmpMapId), "%s#%d", cfgName.c_str(), getMapNumByCfgName(cfgName));
        mapId = tmpMapId;
    }
    if (this->getMap(mapId)){
        return NULL;
    }
    MapObjPtr ret = new MapObj(mapId, mapCfg);
    addMap(ret);
    return ret;
}
void MapMgr::addMap(MapObjPtr& p){
    m_maps[p->getMapId()] = p;
}
bool MapMgr::delMap(const std::string& s){
    std::map<std::string, MapObjPtr>::iterator it = m_maps.find(s);
    if (it != m_maps.end()){
        m_maps.erase(it);
        return true;
    }
    return false;
}
MapObjPtr MapMgr::getMap(const std::string& s){
    std::map<std::string, MapObjPtr>::iterator it = m_maps.find(s);
    if (it != m_maps.end()){
        return it->second;
    }
    return NULL;
}
int MapMgr::getMapNumByCfgName(const std::string& s){
    int num = 0;
    std::map<std::string, MapObjPtr>::iterator it = m_maps.begin();
    for (; it != m_maps.end(); ++it){
        if (it->second->mapCfg->cfgName == s){
            ++num;
        }
    }
    return num;
}

struct MapScriptFunctor{
    static bool enter(EntityPtr p, const string& mapId, int x, int y){
        return MAP_MGR.enter(p, mapId, x, y);
    }
    static bool move(EntityPtr p, int x, int y){
        return p->get<MapCtrl>()->curMap->move(p, x, y);
    }
    static string getEntityMapId(EntityPtr p){
        if (p->get<MapCtrl>()->curMap){
            return p->get<MapCtrl>()->curMap->mapId;
        }
        return "";
    }
    static int getMapNumByCfgName(const string& cfgName){
        return MAP_MGR.getMapNumByCfgName(cfgName);
    }
    static int getAllMapNum(){
        return MAP_MGR.getAllMapNum();
    }
    static MapObjPtr allocMap(const std::string& cfgName, std::string mapId){
        return MAP_MGR.allocMap(cfgName, mapId);
    }
    static bool delMap(const std::string& mapId){
        return MAP_MGR.delMap(mapId);
    }
    static MapObjPtr getMap(const std::string& mapId){
        return MAP_MGR.getMap(mapId);
    }
    static string getMapId(MapObjPtr mapObj){
        return mapObj->mapId;
    }
    static string getMapCfgName(MapObjPtr mapObj){
        return mapObj->mapCfg->cfgName;
    }
    static vector<EntityPtr> getAllEntity(const std::string& mapId, int nType){
        vector<EntityPtr> ret;
        MapObjPtr pMapObj = MAP_MGR.getMap(mapId);
        if (pMapObj){
            pMapObj->getAllEntity(ret, nType);
        }
        return ret;
    }
    static vector<EntityPtr> rangeGetEntities(const std::string& mapId, int x, int y, int radius, bool includeCenter){
        vector<EntityPtr> ret;
        MapObjPtr pMapObj = MAP_MGR.getMap(mapId);
        if (pMapObj){
            ret = pMapObj->rangeGetEntities(x, y, radius, includeCenter);
        }
        return ret;
    }
};
bool MapModule::init(){
    //!注册操作任务的脚本接口
    if (false == MAP_MGR.init()){
        return false;
    }
    SCRIPT_UTIL.reg("Map.enter",              MapScriptFunctor::enter);
    SCRIPT_UTIL.reg("Map.move",               MapScriptFunctor::move);
    SCRIPT_UTIL.reg("Map.getEntityMapId",     MapScriptFunctor::getEntityMapId);
    SCRIPT_UTIL.reg("Map.getMapNumByCfgName", MapScriptFunctor::getMapNumByCfgName);
    SCRIPT_UTIL.reg("Map.getAllMapNum",       MapScriptFunctor::getAllMapNum);
    SCRIPT_UTIL.reg("Map.delMap",             MapScriptFunctor::delMap);
    SCRIPT_UTIL.reg("Map.getMap",             MapScriptFunctor::getMap);
    SCRIPT_UTIL.reg("Map.getMapId",           MapScriptFunctor::getMapId);
    SCRIPT_UTIL.reg("Map.getMapCfgName",      MapScriptFunctor::getMapCfgName);
    SCRIPT_UTIL.reg("Map.getAllEntity",       MapScriptFunctor::getAllEntity);
    SCRIPT_UTIL.reg("Map.rangeGetEntities",   MapScriptFunctor::rangeGetEntities);
    return true;
}
#include "common/entity.h"
#include "server/ffworker.h"
#include "server/script.h"
#include "common/prop.h"

using namespace ff;
using namespace std;

map<long, WeakPtr<Entity> > Entity::EntityPtr2Ref;

SharedPtr<Entity> Entity::genEntity(int ntype, userid_t id, userid_t sid){
    SharedPtr<Entity> ret = new Entity(ntype, id, sid);
    Entity::EntityPtr2Ref[long(ret.get())] = ret;
    return ret;
}
SharedPtr<Entity> Entity::toEntity(long p){
    std::map<long, WeakPtr<Entity> >::iterator it = EntityPtr2Ref.find(p);
    if (it != EntityPtr2Ref.end()){
        return it->second.lock();
    }
    return NULL;
}
Entity::~Entity(){
    Entity::EntityPtr2Ref.erase(long(this));
    clearField();
}

bool Entity::sendMsg(uint16_t cmd, const std::string& msg)
{
    if (m_sessionID){
        FFWORKER_SINGLETON.sessionSendMsg(m_sessionID, cmd, msg);
        return true;
    }
    return false;
}
SharedPtr<Entity> Entity::toPtr(){
    return Entity::toEntity(long(this));
}
void Entity::initField(EntityField* ret, const std::string& name){
    ret->setFiledName(name);
    ret->setOwner(this->toPtr());
}
EntityField* Entity::getFieldByName(const std::string& name){
    for (size_t i = 0; i < m_fields.size(); ++i){
        EntityField* ret = m_fields[i];
        if (ret && ret->getFieldName() == name){
            return ret;
        }
    }
    return NULL;
}
void Entity::clearField(){
    for (size_t i = 0; i < m_fields.size(); ++i){
        EntityField* ret = m_fields[i];
        if (ret){
            m_fields[i] = NULL;
            delete ret;
        }
    }
    m_fields.clear();
}

void EntityMgr::add(EntityPtr& p){
    m_all_entity[p->getType()][p->getUid()] = p;
}
bool EntityMgr::del(int ntype, userid_t id){
    std::map<userid_t, EntityPtr>& allEntity = m_all_entity[ntype];
    std::map<userid_t, EntityPtr>::iterator it = allEntity.find(id);
    if (it != allEntity.end()){
        allEntity.erase(it);
        return true;
    }
    return false;
}
EntityPtr EntityMgr::get(int ntype, userid_t id){
    std::map<userid_t, EntityPtr>& allEntity = m_all_entity[ntype];
    std::map<userid_t, EntityPtr>::iterator it = allEntity.find(id);
    if (it != allEntity.end()){
        return it->second;
    }
    return NULL;
}
static userid_t Entity_getUid(EntityPtr p){
    if (p){
        return p->getUid();
    }
    return 0;
}
static size_t Entity_totalNum(){
    return Entity::EntityPtr2Ref.size();
}
int64_t gInt = 0;
static bool initEntityEnvir(){
    //!这里演示的是如何注册脚本接口
    SCRIPT_UTIL.reg("Entity.getUid", Entity_getUid);
    SCRIPT_UTIL.reg("Entity.totalNum", Entity_totalNum);

    /*example code
    printf("initEntityEnvir....\n");
    
    
    ScriptArgs varScriptArgs;
    SCRIPT_UTIL.callScriptRaw<void>("testScriptCall", varScriptArgs);
    int arg1 = 0;
    string arg2 = "sss";
    double arg3  = 3.14;
    int ret = 0;
    for (int i = 0; i < 1; ++i)
        ret = SCRIPT_UTIL.callScript<int>("testScriptCall", arg1, arg2, arg3, arg1, arg2, arg3, arg1, arg2, arg3);
    printf("initEntityEnvir....callScript:%d\n", ret);
    */
    return true;
}
static bool cleanupEntityEnvir(){
    /*example code
    printf("cleanupEntityEnvir....\n");
    
    SCRIPT_UTIL.callScript<void>("testScriptCall");
    */
    return true;
    
}

WORKER_AT_SETUP(initEntityEnvir);
WORKER_AT_EXIT(cleanupEntityEnvir);



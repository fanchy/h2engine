#ifndef _FF_SKILL_H_
#define _FF_SKILL_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "common/entity.h"
#include "base/smart_ptr.h"

namespace ff
{

class SkillHurtHandler{
public:
    virtual ~SkillHurtHandler(){}
    virtual bool useSkill(int skillid, EntityPtr owner, EntityPtr dest, int destX, int destY, const std::string& arg) = 0;
};
typedef SharedPtr<SkillHurtHandler> SkillHurtHandlerPtr;

template <typename T>
struct SkillHurtHandlerCommon: public SkillHurtHandler
{
    SkillHurtHandlerCommon(T pFuncArg):pFunc(pFuncArg){}
    virtual bool useSkill(int skillid, EntityPtr owner, EntityPtr dest, int destX, int destY, const std::string& arg)
    {
        return pFunc(skillid, owner, dest, destX, destY, arg);
    }
    T pFunc;
};

struct SkillConfig{
    SkillConfig():cfgid(0), mp(0), cd(0), attackDistance(0), costAnger(0){}
    int64_t getPropNum(const std::string& key);
    std::string getPropStr(const std::string& key);

    int             					cfgid;
    std::string                         name;
    int                                 mp;
    int                                 cd;
    int                                 attackDistance;
    int                                 costAnger;
    SkillHurtHandlerPtr                 skillHurtHandler;

    
    std::map<std::string, std::string>  allProp;
};
typedef SharedPtr<SkillConfig> SkillConfigPtr;

class SkillMgr
{
public:
    SkillConfigPtr getCfg(int cfgid) {
    	std::map<int, SkillConfigPtr>::iterator it = m_allSkillCfg.find(cfgid);
    	if (it != m_allSkillCfg.end()){
    		return it->second;
    	}
    	return NULL;
    }

    bool init();
public:
    std::map<int, SkillConfigPtr>    m_allSkillCfg;
};

struct SkillObj
{
    SkillObj(SkillConfigPtr cfg):skillCfg(cfg){}

    SkillConfigPtr      skillCfg;
};
typedef SharedPtr<SkillObj> SkillObjPtr;

class SkillCtrl:public EntityField
{
public:
	SkillCtrl(){}
	
    bool addSkill(SkillConfigPtr cfg);
    bool delSkill(int skillid);
    bool useSkill(int skillid, userid_t targetid, int destX, int destY, const std::string& arg);
    SkillObjPtr getSkill(int skillid);
public:
    std::map<int, SkillObjPtr>      m_allSkills;
};

}

#endif

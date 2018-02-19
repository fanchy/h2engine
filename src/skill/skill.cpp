#include <ctype.h>
#include <algorithm>    // std::transform

#include "skill/skill.h"
#include "server/db_mgr.h"
#include "common/game_event.h"
#include "base/str_tool.h"
#include "server/script.h"
#include "common/game_event.h"
#include "base/os_tool.h"

using namespace ff;
using namespace std;


int64_t SkillConfig::getPropNum(const std::string& key){
    string v = getPropStr(key);
    if (v.empty() == false){
        return ::atol(v.c_str());
    }
    return 0;
}

string SkillConfig::getPropStr(const std::string& key){
    string keyname = key;
    std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::toupper);
    std::map<std::string, std::string>::iterator it = allProp.find(keyname);
    if (it != allProp.end()){
        return it->second;
    }
    return "";
}

struct ScriptSkillHandler{
    ScriptSkillHandler(const string& s):scriptFuncName(s){}
    bool operator()(int skillid, EntityPtr owner, EntityPtr dest, int destX, int destY, const std::string& arg)
    {
        return SCRIPT_UTIL.callScript<bool>(scriptFuncName, skillid, owner, dest, destX, destY, arg);
    }
    string scriptFuncName;
};
bool SkillMgr::init(){
    vector<vector<string> > retdata;
    string csvData;
    OSTool::readFile("config/skill_config.csv", csvData);
    StrTool::loadCsvFromString(csvData, retdata);

    if (retdata.empty()){
        LOGWARN((GAME_LOG, "MapMgr::init load none data"));
        return true;
    }
    vector<string>& col = retdata[0];
    for (size_t i = 1; i < retdata.size(); ++i){
        vector<string>& row = retdata[i];
        SkillConfigPtr skillCfg = new SkillConfig();

        for (size_t j = 0; j < row.size(); ++j){
            string& keyname = col[j];
            std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::toupper);
            skillCfg->allProp[keyname] = row[j];
        }
        skillCfg->cfgid   = (int)skillCfg->getPropNum("cfgid");
        skillCfg->name             = skillCfg->getPropStr("name");
        skillCfg->mp               = (int)skillCfg->getPropNum("mp");
        skillCfg->cd               = (int)skillCfg->getPropNum("cd");
        skillCfg->attackDistance   = (int)skillCfg->getPropNum("attackDistance");
        skillCfg->costAnger        = (int)skillCfg->getPropNum("costAnger");

        string script = skillCfg->getPropStr("script");
        if (script.empty() == false){
            ScriptSkillHandler func(script);
            skillCfg->skillHurtHandler = new SkillHurtHandlerCommon<ScriptSkillHandler>(func);
        }
        
        m_allSkillCfg[skillCfg->cfgid] = skillCfg;
    }
    return true;
}

bool SkillCtrl::addSkill(SkillConfigPtr cfg){
    SkillObjPtr skill = new SkillObj(cfg);
    m_allSkills[cfg->cfgid] = skill;
    return true;
}

bool SkillCtrl::delSkill(int skillid){
    std::map<int, SkillObjPtr>::iterator it = m_allSkills.find(skillid);
    if (it != m_allSkills.end()){
        m_allSkills.erase(it);
        return true;
    }
    return false;
}
SkillObjPtr SkillCtrl::getSkill(int skillid){
    std::map<int, SkillObjPtr>::iterator it = m_allSkills.find(skillid);
    if (it != m_allSkills.end()){
        return it->second;
    }
    return NULL;
}
bool SkillCtrl::useSkill(int skillid, userid_t targetid, int destX, int destY, const std::string& arg){
    SkillObjPtr skill = getSkill(skillid);
    if (!skill){
        return false;
    }
    EntityPtr owner = this->getOwner();
    EntityPtr dest  = ENTITY_MGR.get(ENTITY_PLAYER, targetid);
    if (!dest){
        return false;
    }
    
    if (skill->skillCfg->skillHurtHandler){
        if (skill->skillCfg->skillHurtHandler->useSkill(skillid, owner, dest, destX, destY, arg)){
            return true;
        }
    }
    return false;
}

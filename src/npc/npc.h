#ifndef _FF_NPC_H_
#define _FF_NPC_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "common/entity.h"
#include "base/smart_ptr.h"

namespace ff
{

struct NPCConfig{
    NPCConfig():cfgid(0), x(0), y(0), bValid(true){}
    std::string getPropStr(const std::string& key);

    int             					cfgid;
    std::string     					name;
    std::string     					strMap;
    int             					x;
	int 								y;
	std::string     					strScript;
    std::map<std::string, std::string>  strProp;
    bool 								bValid;
};
typedef SharedPtr<NPCConfig> NPCConfigPtr;

class NPCMgr
{
public:
    NPCConfigPtr getCfg(int cfgid) {
    	std::map<int, NPCConfigPtr>::iterator it = m_NPCCfgs.find(cfgid);
    	if (it != m_NPCCfgs.end()){
    		return it->second;
    	}
    	return NULL;
    }

	EntityPtr addNPC(NPCConfigPtr cfg);
    bool init();
public:
    std::map<int, NPCConfigPtr>    m_NPCCfgs;
};


class NPCCtrl:public EntityField
{
public:
	NPCCtrl():m_curNPCUid(0){}
	EntityPtr getCurNPC();
	bool processNPC(EntityPtr npc, const std::string& arg);

public:
    userid_t        m_curNPCUid;
};

}

#endif

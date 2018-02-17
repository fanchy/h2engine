#include <ctype.h>

#include "npc/npc.h"
#include "server/db_mgr.h"
#include "common/game_event.h"
#include "base/str_tool.h"
#include "server/script.h"

using namespace ff;
using namespace std;


bool NPCMgr::init(){
	return true;
}
EntityPtr NPCMgr::addNPC(NPCConfigPtr cfg){
	static int nId = 0;
	EntityPtr eNPC = NEW_ENTITY(ENTITY_NPC, ++nId);
	return eNPC;
}

bool NPCCtrl::processNPC(EntityPtr npc, const std::string& arg)
{
	vector<string> vt;
	StrTool::split(arg, vt, "?");
	
	string funcName;
	map<string, string> paramKey2Value;
	if (vt.empty()){
		funcName = "main";
	}
	else{
		funcName = vt[0];
		string strCheck = "\"" + funcName + "\"";
		if (m_strLastNpcText.find(strCheck) == string::npos){
			LOGERROR((GAME_LOG, "npc check failed %s", strCheck));
			return false;
		}
		if (vt.size() > 1){
			vector<string> params;
			StrTool::split(vt[1], params, "&");
			
			vector<string> pairArg;
			for (size_t i = 0; i < params.size(); ++i){
				StrTool::split(params[i], pairArg, "=");
				pairArg.clear();
				if (pairArg.size() == 2){
					paramKey2Value[pairArg[0]] = pairArg[1];
				}
				else{
					paramKey2Value[pairArg[0]] = "";
				}
				
			}
		}
	}
	m_curNPCUid = npc->getUid();
	NPCConfigPtr cfg;
	if (!cfg)
		return false;
	try{
		if (cfg->strScript.empty())
			return false;
		string callFunc;
		if (cfg->strScript[cfg->strScript.size() - 1] == '_' || cfg->strScript[cfg->strScript.size() - 1] == '.'){
			callFunc = cfg->strScript + funcName;
		}
		else {
			callFunc = cfg->strScript + "." + funcName;
		}
		EntityPtr owner = this->getOwner();
		SCRIPT_UTIL.callScript<void>(callFunc, owner, npc, paramKey2Value);
	}
	catch(exception& e_){
        LOGERROR((GAME_LOG, "npc callScript exception<%s>", e_.what()));
        //exceptInfo = e_.what();
        return false;
    }
	return true;
}

EntityPtr NPCCtrl::getCurNPC() { return UID_TO_ENTITY(ENTITY_NPC, m_curNPCUid); }




#include "server/ffworker.h"
#include "player/player.h"

using namespace ff;
using namespace std;

static bool initEnvir(){
    PROP_MGR.regGetterSetter(PROP_ATTACK     , &PlayerProp::Attack      );      
    PROP_MGR.regGetterSetter(PROP_ATTACKMAGIC, &PlayerProp::AttackMagic );      
    PROP_MGR.regGetterSetter(PROP_DEFEND     , &PlayerProp::Defend      );      
    PROP_MGR.regGetterSetter(PROP_DEFENDMAGIC, &PlayerProp::DefendMagic );      
    PROP_MGR.regGetterSetter(PROP_HP         , &PlayerProp::HP          );      
    PROP_MGR.regGetterSetter(PROP_MP         , &PlayerProp::MP          );      
    PROP_MGR.regGetterSetter(PROP_HIT        , &PlayerProp::Hit         );      
    PROP_MGR.regGetterSetter(PROP_HITMAGIC   , &PlayerProp::HitMagic    );      
    PROP_MGR.regGetterSetter(PROP_AVOID      , &PlayerProp::Avoid       );      
    PROP_MGR.regGetterSetter(PROP_AVOIDMAGIC , &PlayerProp::AvoidMagic  );      
    
    return true;
}

WORKER_AT_SETUP(initEnvir);

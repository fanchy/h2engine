
#include "server/ffworker.h"
#include "player/player.h"

using namespace ff;
using namespace std;

static bool initEnvir(){
    ENTITY_REG_PROP_FIELD("nAttack", &PlayerProp::nAttack);
    return true;
}

WORKER_AT_SETUP(initEnvir);


#include "server/ffworker.h"
#include "common/entity.h"
#include "common/cmdmgr.h"
#include "player/player.h"
#include "map/map.h"
#include "task/task.h"

using namespace ff;
using namespace std;

static bool initEnvir(){
    EntityModule::init();
    CmdModule::init();
    PlayerModule::init();
    MapModule::init();
    TaskModule::init();
    return true;
}
WORKER_AT_SETUP(initEnvir);

static bool cleanupEnvir(){
    return true;
    
}
WORKER_AT_EXIT(cleanupEnvir);

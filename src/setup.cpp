
#include "server/ffworker.h"
#include "server/entity.h"
#include "player/player.h"
#include "map/map.h"
#include "task/task.h"
#include "chat/chat.h"

using namespace ff;
using namespace std;

static bool initEnvir(){
    PlayerModule::init();
    MapModule::init();
    TaskModule::init();
    ChatModule::init();
    return true;
}
WORKER_AT_SETUP(initEnvir);

static bool cleanupEnvir(){
    return true;
    
}
WORKER_AT_EXIT(cleanupEnvir);

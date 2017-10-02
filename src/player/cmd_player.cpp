#include "player/cmd_player.h"

using namespace ff;
using namespace std;

void PlayerService::handleLogin(userid_t sessionId, const std::string& data){
    //!logic code
}
#define LOGIN_CMD  1024
static bool initEnvir(){
    FFWORKER_SINGLETON.regSessionReq(LOGIN_CMD, &PlayerService::handleLogin, &(PLAYER_SERVICE));
    return true;
}

WORKER_AT_SETUP(initEnvir);

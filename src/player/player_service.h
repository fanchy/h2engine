#ifndef _FF_PLAYER_SERVICE_H_
#define _FF_PLAYER_SERVICE_H_

#include "server/ffworker.h"

//!-------example file
namespace ff
{
class PlayerService{
public:
    void handleLogin(userid_t sessionId, const std::string& data);
};

#define PlayerServiceSingleton Singleton<PlayerService>::instance()

}


#endif

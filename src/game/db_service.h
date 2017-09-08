#ifndef _FF_DB_SERVICE_H_
#define _FF_DB_SERVICE_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "game/entity.h"
#include "game/game_event.h"
#include "base/smart_ptr.h"
#include "server/db_mgr.h"

namespace ff
{

class DbService{
public:
    DbService();
    ~DbService();
    bool init();
    bool cleanup();

};
#define DbServiceSingleton Singleton<DbService>::instance()

}
#endif

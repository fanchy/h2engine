
#include "server/ffworker.h"
#include "player/player.h"

using namespace ff;
using namespace std;

struct PropGetterSertter{
    static int64_t get_nAttack(EntityPtr e){ return (int64_t)e->get<PlayerProp>()->nAttack;}
    static void    set_nAttack(EntityPtr e, int64_t n)     { e->get<PlayerProp>()->nAttack = n; }
};
static bool initEnvir(){
    PROP_MGR.regGetterSetter("nAttack", &PropGetterSertter::get_nAttack, &PropGetterSertter::set_nAttack);
    return true;
}

WORKER_AT_SETUP(initEnvir);

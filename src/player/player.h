#ifndef _FF_PLAYER_H_
#define _FF_PLAYER_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "server/entity.h"
#include "base/smart_ptr.h"

namespace ff{
#define PROP_ATTACK          "Attack"
#define PROP_ATTACKMAGIC     "AttackMagic"
#define PROP_DEFEND          "Defend"
#define PROP_DEFENDMAGIC     "DefendMagic"
#define PROP_HP              "HP"
#define PROP_MP              "MP"
#define PROP_HIT             "Hit"
#define PROP_HITMAGIC        "HitMagic"
#define PROP_AVOID           "Avoid"
#define PROP_AVOIDMAGIC      "AvoidMagic"

class PlayerProp:public EntityField
{
public:
    PlayerProp(){}

public:
    int Attack;
    int AttackMagic;
    int Defend;
    int DefendMagic;
    int HP;
    int MP;
    int Hit;
    int HitMagic;
    int Avoid;
    int AvoidMagic;
};

struct PlayerModule{
    static bool init();
};
}
#endif

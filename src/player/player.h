#ifndef _FF_PLAYER_H_
#define _FF_PLAYER_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>

#include "common/entity.h"
#include "base/smart_ptr.h"

namespace ff{
class PlayerProp:public EntityField
{
public:
    PlayerProp(){}

public:
    int64_t nAttack;
    int64_t nAttackMagic;
    int64_t nDefend;
    int64_t nDefendMagic;
    int64_t nHP;
    int64_t nMP;
    int64_t nHit;
    int64_t nHItMagic;
    int64_t nAvoid;
    int64_t nAvoidMagic;
};
}
#endif

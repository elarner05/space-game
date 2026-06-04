#include "Stats.h"

bool Stats::dealDamage(int16_t dmg) {
    health -= dmg;
    return health <= 0;
}
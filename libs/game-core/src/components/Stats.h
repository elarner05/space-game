#pragma once
#include <cstdint>

struct Stats {
    int16_t health;

    bool dealDamage(int16_t dmg); // returns whether the health has reached zero
};
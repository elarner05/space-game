#pragma once
#include "stdint.h"

enum class EntityTag : uint8_t
{
    Spaceship = 0,
    Asteroid
};

enum class EntityFlags : uint8_t {
    None        = 0,
    Persistent  = 1 << 0,  // never saved to chunk files
    // could have things such as static, noCollision etc
};

constexpr EntityFlags operator&(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
    );
}

constexpr EntityFlags operator|(EntityFlags a, EntityFlags b) {
    return static_cast<EntityFlags>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
    );
}

struct EntityType {
    const char* textureFilepath;
    const char* metaFilepath;
    const char* colliderFilepath;
};

namespace EntityTypes {
    constexpr EntityType Spaceship = {
        "data/spaceship-1.png",
        "data/spaceship-1.meta",
        "data/spaceship-1-collider.meta"
    };
    constexpr EntityType Asteroid = {
        "data/asteroid.png",
        "data/asteroid.meta",
        "data/asteroid-collider.meta"
    };
};
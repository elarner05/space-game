#pragma once
#include "Kinematics.h"
#include "EntityID.h"


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

namespace Core {
    EntityID registerEntity(const EntityType type);
    EntityID registerEntity(const EntityType type, Kinematics kin);
}
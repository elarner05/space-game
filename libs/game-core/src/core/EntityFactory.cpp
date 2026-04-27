#include "core/EntityFactory.h"
#include "components/Animation.h"
#include "components/Animation.h"
#include "components/CompoundCollider.h"
#include "core/Core.h"
#include "utils/TextureManager.h"
#include <iostream>

static EntityID registerEntity(EntityTag tag, const EntityType type, Kinematics kin, EntityFlags flags) {
    CompoundCollider col;
    if (!col.loadColliders(type.colliderFilepath)) {
        std::cerr << "Failed to load collider for entity type: " << type.colliderFilepath << std::endl;
    }

    Animations anim;
    if (!anim.loadAnimations(type.metaFilepath)) {
        std::cerr << "Failed to load animation metadata for entity type: " << type.metaFilepath << std::endl;
    }

    Texture2D tex = TextureManager::loadTexture(std::string(type.textureFilepath));
    if (tex.id == 0) {
        std::cerr << "Failed to load texture for entity type: " << type.textureFilepath << std::endl;
    }

    return Core::registerEntity(tag, kin, col, anim, tex, flags);
}


EntityID Core::EntityFactory::spawn(EntityTag tag, EntityFlags flags) {
    switch (tag) {
        case EntityTag::Spaceship:
            return registerEntity(tag, EntityTypes::Spaceship, Kinematics{}, flags);
        case EntityTag::Asteroid:
            return registerEntity(tag, EntityTypes::Asteroid, Kinematics{}, flags);
        default:
            return EntityID::invalid();
    }
}
EntityID Core::EntityFactory::spawn(EntityTag tag, Kinematics kin, EntityFlags flags) {
    switch (tag) {
        case EntityTag::Spaceship:
            return registerEntity(tag, EntityTypes::Spaceship, kin, flags);
        case EntityTag::Asteroid:
            return registerEntity(tag, EntityTypes::Asteroid, kin, flags);
        default:
            return EntityID::invalid();
    }
}
#include "Entity.h"
#include "Animation.h"
#include "Animation.h"
#include "CompoundCollider.h"
#include "Core.h"
#include <iostream>

namespace Core {
    EntityID registerEntity(const EntityType type) {
        Kinematics kin;
        kin.chunk = { 0, 0 };
        kin.localPosition = { 0.f, 0.f };
        kin.velocity = { 0.f, 0.f };
        kin.mass = 1.f;
        kin.rotation = 0.f;
        kin.angularVelocity = 0.f;

        return registerEntity(type, kin);
    }
    EntityID registerEntity(const EntityType type, Kinematics kin) {
        CompoundCollider col;
        if (!col.loadColliders(type.colliderFilepath)) {
            std::cerr << "Failed to load collider for entity type: " << type.colliderFilepath << std::endl;
        }

        Animations anim;
        if (!anim.loadAnimations(type.metaFilepath)) {
            std::cerr << "Failed to load animation metadata for entity type: " << type.metaFilepath << std::endl;
        }

        Texture2D tex = LoadTexture(type.textureFilepath);
        if (tex.id == 0) {
            std::cerr << "Failed to load texture for entity type: " << type.textureFilepath << std::endl;
        }

        return registerEntity(kin, col, anim, tex);
    }
}
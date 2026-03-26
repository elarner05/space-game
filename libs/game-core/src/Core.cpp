#include "Core.h"

namespace Core {
    AnimationSystem animationSystem;
    KinematicsSystem kinematicsSystem;
    ColliderSystem colliderSystem;

    void init() {
        animationSystem = AnimationSystem{};
        kinematicsSystem = KinematicsSystem{};
        colliderSystem = ColliderSystem{}; 
    }

    void update(float dt) {
        kinematicsSystem.update(dt);
        colliderSystem.syncPositions(kinematicsSystem.getPositions());
        colliderSystem.update(dt);

        animationSystem.update(dt);
    }

    CompoundCollider* registerComponent(CompoundCollider component) {
        CompoundCollider* t = colliderSystem.registerEntity(component);
        // t.loadColliders("resources/spaceship-1-collider.meta");
        return t;
    }
    Kinematics* registerComponent(Kinematics component) {
        return kinematicsSystem.registerEntity(component);
    }
    Animations* registerComponent(Animations component) {
        return animationSystem.registerEntity(component);
    
    }

    void unloadAll() {
        colliderSystem.~ColliderSystem();
        kinematicsSystem.~KinematicsSystem();
        animationSystem.~AnimationSystem();

        TextureManager::unloadAllTextures();
    }
    
}
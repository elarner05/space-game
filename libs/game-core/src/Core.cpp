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

        processCollisions(dt);

        animationSystem.update(dt);
    }

    bool handleCollision(Kinematics& kinA, Kinematics& kinB, CompoundCollider& colA, CompoundCollider& colB) {
        bool handled = false;

        constexpr float elasticity = 1.f;

        for (int i = 0; i < colA.colliderCount; i++) {
            for (int j = 0; j < colB.colliderCount; j++) {

                if (!withinBounds(colA.colliders[i], kinA.position, colB.colliders[j], kinB.position))
                    continue;

                ContactManifold manifold = {};

                if (colA.colliders[i].count == 0 && colB.colliders[j].count == 0) {
                    // Circle-circle: bypass GJK/EPA, compute manifold directly
                    const Collider& cA = colA.colliders[i];
                    const Collider& cB = colB.colliders[j];
                    Vector2 centerA = Vector2Add(kinA.position, cA.offset);
                    Vector2 centerB = Vector2Add(kinB.position, cB.offset);
                    Vector2 delta   = Vector2Subtract(centerA, centerB);
                    float   dist    = Vector2Length(delta);

                    manifold.normal  = (dist > 1e-6f) ? Vector2Normalize(delta) : Vector2{ 1.f, 0.f };
                    manifold.depth   = (cA.radius + cB.radius) - dist;
                    manifold.contact = Vector2Add(centerB, Vector2Scale(manifold.normal, cB.radius));
                    manifold.valid   = true;
                } else {
                    // Polygon or mixed: use GJK + EPA
                    Vector2 simplex[3] = {};
                    if (!gjk(colA.colliders[i], kinA.position, colB.colliders[j], kinB.position, simplex))
                        continue;

                    manifold = epa(colA.colliders[i], kinA.position, colB.colliders[j], kinB.position, simplex);
                }

                if (!manifold.valid)
                    continue;

                resolveCollision(manifold, &kinA, &kinB, elasticity);
                positionalCorrection(manifold, &kinA, &kinB);
                handled = true;
            }
        }

        return handled;
    }

    
    void processCollisions(float dt) {
        for (size_t i = 0; i < kinematicsSystem.m_entities.size(); ++i) {
            for (size_t j = i + 1; j < kinematicsSystem.m_entities.size(); ++j) {
                Kinematics& kinA = kinematicsSystem.m_entities[i];
                Kinematics& kinB = kinematicsSystem.m_entities[j];
                CompoundCollider& colA = colliderSystem.m_entities[i];
                CompoundCollider& colB = colliderSystem.m_entities[j];

                handleCollision(kinA, kinB, colA, colB);
            }
        }
    }
        
 

    CompoundCollider* registerComponent(CompoundCollider component) {
        return colliderSystem.registerEntity(component);
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
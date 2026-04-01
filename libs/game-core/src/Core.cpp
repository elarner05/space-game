#include "Core.h"
#include "debug_flags.h"
namespace Core {
    AnimationSystem animationSystem;
    KinematicsSystem kinematicsSystem;
    ColliderSystem colliderSystem;
    RenderSystem renderSystem;
    GameCamera camera;

    void init() {
        animationSystem = AnimationSystem{};
        kinematicsSystem = KinematicsSystem{};
        colliderSystem = ColliderSystem{};
    }

    

    void update(float dt) {
        kinematicsSystem.update(dt);
        colliderSystem.update(dt);

        processCollisions(dt);

        camera.updatePosition(dt);

        animationSystem.update(dt);
    }

    void draw() {
        if ( Debug::showChunkBounds() ) {
            renderChunkBoundaries();
        }

        for (size_t i = 0; i < animationSystem.m_entities.size(); ++i) {
            Animations& anim = animationSystem.m_entities[i];
            Kinematics& kin = kinematicsSystem.m_entities[i];
            CompoundCollider& col = colliderSystem.m_entities[i];
            Texture2D& tex = renderSystem.m_entities[i];

            Vector2 screenPos = kin.localPositionRelativeTo(
                camera.kinematics.chunk,
                camera.kinematics.localPosition
            );

            DrawTexturePro(tex, anim.getSource(),
                Rectangle{ screenPos.x, screenPos.y, anim.dimensions.x, anim.dimensions.y },
                Vector2{ anim.origin.x, anim.origin.y },
                (float)kin.rotation * RAD2DEG, RAYWHITE);


            if (Debug::showHitboxes())
                col.drawDebug(screenPos, RED);
            if (Debug::showEntityOrigins())
                DrawCircle(screenPos.x, screenPos.y, 2, RED);
            if (Debug::showVelocities()) {
                Vector2 velocityEnd = Vector2Add(screenPos, Vector2Scale(kin.velocity, 0.1f));
                DrawLineV(screenPos, velocityEnd, BLUE);
            }
        }

    }

    bool handleCollision(Kinematics& kinA, Kinematics& kinB, CompoundCollider& colA, CompoundCollider& colB){
        bool handled = false;
        constexpr float elasticity = 1.f;

        // Express both entities relative to the camera's chunk/position
        Vector2 posA = kinA.localPositionRelativeTo(camera.kinematics.chunk, camera.kinematics.localPosition);
        Vector2 posB = kinB.localPositionRelativeTo(camera.kinematics.chunk, camera.kinematics.localPosition);

        for (int i = 0; i < colA.colliderCount; i++) {
            for (int j = 0; j < colB.colliderCount; j++) {
                if (!withinBounds(colA.colliders[i], posA, colB.colliders[j], posB))
                    continue;

                ContactManifold manifold = {};

                if (colA.colliders[i].count == 0 && colB.colliders[j].count == 0) {
                    // Circle-circle: bypass GJK/EPA
                    const Collider& cA = colA.colliders[i];
                    const Collider& cB = colB.colliders[j];
                    Vector2 centerA = Vector2Add(posA, cA.offset);
                    Vector2 centerB = Vector2Add(posB, cB.offset);
                    Vector2 delta   = Vector2Subtract(centerA, centerB);
                    float   dist    = Vector2Length(delta);
                    manifold.normal  = (dist > 1e-6f) ? Vector2Normalize(delta) : Vector2{ 1.f, 0.f };
                    manifold.depth   = (cA.radius + cB.radius) - dist;
                    manifold.contact = Vector2Add(centerB, Vector2Scale(manifold.normal, cB.radius));
                    manifold.valid   = manifold.depth > 0.f;
                } else {
                    // Polygon or mixed: GJK + EPA
                    Vector2 simplex[3] = {};
                    if (!gjk(colA.colliders[i], posA, colB.colliders[j], posB, simplex))
                        continue;
                    manifold = epa(colA.colliders[i], posA, colB.colliders[j], posB, simplex);
                }

                if (!manifold.valid) continue;

                resolveCollision(manifold, &kinA, posA, &kinB, posB, elasticity);
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

                // Cheap chunk cull before anything else
                if (!kinA.chunk.isAdjacentTo(kinB.chunk)) continue;

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
    Texture2D* registerComponent(Texture2D component) {
        return renderSystem.registerEntity(component);
    }


    void renderChunkBoundaries(Color color) {
        const Kinematics& cam = Core::camera.kinematics;

        // how many chunks fit on screen in each direction
        int chunksX = (int)ceilf((GetScreenWidth()  * 0.5f) / CHUNK_SIZEF) + 1;
        int chunksY = (int)ceilf((GetScreenHeight() * 0.5f) / CHUNK_SIZEF) + 1;

        // pixel offset of the camera within its current chunk
        float offsetX = cam.localPosition.x;
        float offsetY = cam.localPosition.y;

        // screen centre
        float cx = GetScreenWidth()  * 0.5f;
        float cy = GetScreenHeight() * 0.5f;

        // vertical lines
        for (int dx = -chunksX; dx <= chunksX; dx++) {
            float screenX = cx + (dx * CHUNK_SIZEF) - offsetX;
            DrawLine((int)screenX, 0, (int)screenX, GetScreenHeight(), color);
        }

        // horizontal lines
        for (int dy = -chunksY; dy <= chunksY; dy++) {
            float screenY = cy + (dy * CHUNK_SIZEF) - offsetY;
            DrawLine(0, (int)screenY, GetScreenWidth(), (int)screenY, color);
        }
    }

    void unloadAll() {
        colliderSystem.~ColliderSystem();
        kinematicsSystem.~KinematicsSystem();
        animationSystem.~AnimationSystem();

        TextureManager::unloadAllTextures();
    }
    
}
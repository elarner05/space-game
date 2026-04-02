#include "Core.h"
#include "debug_flags.h"
using EntityID = size_t;
namespace Core {
    AnimationSystem animationSystem;
    KinematicsSystem kinematicsSystem;
    ColliderSystem colliderSystem;
    RenderSystem renderSystem;
    GameCamera camera;

    robin_hood::unordered_map<ChunkCoord, std::vector<EntityID>> chunkMap;

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

    void handleCollision(EntityID a, EntityID b) {
        handleCollision(
            kinematicsSystem.m_entities[a], kinematicsSystem.m_entities[b],
            colliderSystem.m_entities[a],   colliderSystem.m_entities[b]
        );
    }

    void processCollisions(float dt) {
        ChunkCoord camChunk = camera.kinematics.chunk;
        int simDist = GameCamera::simulationDistance;

        // track tested pairs to avoid duplicates across chunk neighbours
        robin_hood::unordered_set<uint64_t> testedPairs;

        auto pairKey = [](EntityID a, EntityID b) -> uint64_t {
            if (a > b) std::swap(a, b);
            return ((uint64_t)a << 32) | (uint64_t)b;
        };

        for (int dx = -simDist; dx <= simDist; dx++) {
            for (int dy = -simDist; dy <= simDist; dy++) {
                ChunkCoord chunk = { camChunk.x + dx, camChunk.y + dy };
                auto it = chunkMap.find(chunk);
                if (it == chunkMap.end()) continue;

                const std::vector<EntityID>& locals = it->second;

                // test within this chunk
                for (size_t i = 0; i < locals.size(); i++) {
                    for (size_t j = i + 1; j < locals.size(); j++) {
                        EntityID a = locals[i], b = locals[j];
                        if (testedPairs.insert(pairKey(a, b)).second)
                            handleCollision(a, b);
                    }
                }

                // test against each neighbour chunk
                for (int nx = -1; nx <= 1; nx++) {
                    for (int ny = -1; ny <= 1; ny++) {
                        if (nx == 0 && ny == 0) continue;
                        ChunkCoord neighbour = { chunk.x + nx, chunk.y + ny };
                        auto nit = chunkMap.find(neighbour);
                        if (nit == chunkMap.end()) continue;

                        for (EntityID a : locals) {
                            for (EntityID b : nit->second) {
                                if (testedPairs.insert(pairKey(a, b)).second)
                                    handleCollision(a, b);
                            }
                        }
                    }
                }
            }
        }
    }
        
    
 

    CompoundCollider* registerComponent(CompoundCollider component) {
        return colliderSystem.registerEntity(component);
    }

    Kinematics* registerComponent(Kinematics component) {
        EntityID id = kinematicsSystem.m_entities.size();
        chunkMap[component.chunk].push_back(id);
        return kinematicsSystem.registerEntity(component);
    }

    Animations* registerComponent(Animations component) {
        return animationSystem.registerEntity(component);
    
    }
    Texture2D* registerComponent(Texture2D component) {
        return renderSystem.registerEntity(component);
    }


    void renderChunkBoundaries(Color color) {
        float cx = GetScreenWidth();
        float cy = GetScreenHeight();

        const Kinematics& cam = Core::camera.kinematics;


        int hx = cam.chunk.x + GetScreenWidth() / CHUNK_SIZE + 3;
        int hy = cam.chunk.y + GetScreenHeight() / CHUNK_SIZE + 3;
        // std::cout << "Rendering chunk bounds from (" << sx << ", " << sy << ") with size (" << hx << ", " << hy << ")\n";

        for (int x = cam.chunk.x; x < hx; x++) {
            DrawLine((int)(x * CHUNK_SIZE - (cam.chunk.x *CHUNK_SIZE + cam.localPosition.x)), (int)((cam.chunk.y) * CHUNK_SIZE - (cam.chunk.y * CHUNK_SIZE + cam.localPosition.y)),
                     (int)(x * CHUNK_SIZE - (cam.chunk.x *CHUNK_SIZE + cam.localPosition.x)), (int)((hy) * CHUNK_SIZE - (cam.chunk.y * CHUNK_SIZE +  cam.localPosition.y)), color);
        }

        for (int y = cam.chunk.y; y < hy; y++) {
            DrawLine((int)((cam.chunk.x) * CHUNK_SIZE - (cam.chunk.x *CHUNK_SIZE + cam.localPosition.x)), (int)(y * CHUNK_SIZE - (cam.chunk.y * CHUNK_SIZE + cam.localPosition.y)),
                     (int)((hx) * CHUNK_SIZE - (cam.chunk.x *CHUNK_SIZE + cam.localPosition.x)), (int)(y * CHUNK_SIZE - (cam.chunk.y * CHUNK_SIZE + cam.localPosition.y)), color);
        }

    }

    void unloadAll() {
        colliderSystem.~ColliderSystem();
        kinematicsSystem.~KinematicsSystem();
        animationSystem.~AnimationSystem();

        TextureManager::unloadAllTextures();
    }
    
}
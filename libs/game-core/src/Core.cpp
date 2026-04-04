#include "Core.h"
#include "debug_flags.h"
#include "InputHandler.h"

namespace Core {
    AnimationSystem animationSystem;
    KinematicsSystem kinematicsSystem;
    ColliderSystem colliderSystem;
    RenderSystem renderSystem;
    GameCamera camera;

    robin_hood::unordered_map<ChunkCoord, std::vector<EntityID>> chunkMap;
    std::vector<size_t> entityIndexTable;
    std::vector<EntityID> indexToEntity;

    void init() {
        constexpr size_t expectedEntities = 1024; // reserve more memory as the number of entities grows, better allocation startegy

        kinematicsSystem.m_entities.reserve(expectedEntities);
        colliderSystem.m_entities.reserve(expectedEntities);
        animationSystem.m_entities.reserve(expectedEntities);
        renderSystem.m_entities.reserve(expectedEntities);
        indexToEntity.reserve(expectedEntities);
        entityIndexTable.reserve(expectedEntities);
    }

    void update(float dt) {
        Input::handleSpaceshipInput(EntityID{0}, dt); // for now just control the first entity, should be fine for testing
        
        kinematicsSystem.update(dt);
        colliderSystem.update(dt);

        processCollisions(dt);

        camera.updatePosition(dt);

        animationSystem.update(dt);
    }

    // draw function for entities, may need moved to a render system style approach
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
            if (Core::Debug::showVelocities()) {
                Vector2 start = screenPos;
                Vector2 end   = { start.x + kin.velocity.x, start.y + kin.velocity.y };
                DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, BLUE);
                Vector2 dir  = Vector2Normalize(Vector2Subtract(end, start));
                Vector2 perp = { -dir.y, dir.x };
                constexpr float HEAD_LENGTH = 8.f;
                constexpr float HEAD_WIDTH  = 4.f;
                Vector2 tip   = end;
                Vector2 base  = Vector2Subtract(tip, Vector2Scale(dir, HEAD_LENGTH));
                Vector2 left  = Vector2Add(base, Vector2Scale(perp, HEAD_WIDTH));
                Vector2 right = Vector2Subtract(base, Vector2Scale(perp, HEAD_WIDTH));
                DrawTriangle(tip, right, left, BLUE);
            }
        }

    }

    // collision handling, needs refactored into a proper system approach
    bool handleCollision(Kinematics& kinA, Kinematics& kinB, CompoundCollider& colA, CompoundCollider& colB){
        bool handled = false;
        constexpr float elasticity = 1.f;

        // Express both entities relative to the camera's position
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

    // helper to call by ID instead of components
    void handleCollision(EntityID a, EntityID b) {
        handleCollision(
            kinematicsSystem.m_entities[static_cast<size_t>(a)], kinematicsSystem.m_entities[static_cast<size_t>(b)],
            colliderSystem.m_entities[static_cast<size_t>(a)],   colliderSystem.m_entities[static_cast<size_t>(b)]
        );
    }

    // needs refactored into system
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
        
    
 
    // registers all the components of an entity to the system
    EntityID registerEntity(Kinematics kin, CompoundCollider col, Animations anim, Texture2D tex) {
        EntityID id = EntityID{static_cast<u_int32_t>(kinematicsSystem.m_entities.size())};
        
        kinematicsSystem.registerEntity(kin);
        colliderSystem.registerEntity(col);
        animationSystem.registerEntity(anim);
        renderSystem.registerEntity(tex);
        
        chunkMap[kin.chunk].push_back(id);

        kinematicsSystem.m_entities.back().computeAndSetBoundingRadius(colliderSystem.m_entities.back()); // needed for epa stage
        return id;
    }

    // removes (deletes) an entity from all systems
    // maintains indirection table integrity for cache performance (does not reuse entityIDs)
    void unregisterEntity(EntityID id) {
        size_t idx = entityIndexTable[id];
        size_t lastIdx = kinematicsSystem.m_entities.size() - 1;
        EntityID lastEntity = indexToEntity[lastIdx];

        // swap components
        std::swap(kinematicsSystem.m_entities[idx], kinematicsSystem.m_entities[lastIdx]);
        std::swap(animationSystem.m_entities[idx], animationSystem.m_entities[lastIdx]);
        std::swap(renderSystem.m_entities[idx], renderSystem.m_entities[lastIdx]);
        std::swap(colliderSystem.m_entities[idx], colliderSystem.m_entities[lastIdx]);

        // fix indirection table
        entityIndexTable[lastEntity] = idx;
        indexToEntity[idx] = lastEntity;

        kinematicsSystem.m_entities.pop_back();
        animationSystem.m_entities.pop_back();
        renderSystem.m_entities.pop_back();
        colliderSystem.m_entities.pop_back();

        indexToEntity.pop_back();
        
        // mark id as dead
        entityIndexTable[id] = SIZE_MAX;
    }

    // helpers to get components by ID, needs moved into systems
    Kinematics& getKinematics(EntityID id) {
        return kinematicsSystem.m_entities[static_cast<size_t>(id)];
    }
    CompoundCollider& getCollider(EntityID id) {
        return colliderSystem.m_entities[static_cast<size_t>(id)];
    }
    Animations& getAnimations(EntityID id) {
        return animationSystem.m_entities[static_cast<size_t>(id)];
    }
    Texture2D& getTexture(EntityID id) {
        return renderSystem.m_entities[static_cast<size_t>(id)];
    }

    // needs moved to render system, useful for debug
    void renderChunkBoundaries(Color color) {
        float cx = GetScreenWidth();
        float cy = GetScreenHeight();

        const Kinematics& cam = Core::camera.kinematics;


        int hx = cam.chunk.x + GetScreenWidth() / CHUNK_SIZE + 3;
        int hy = cam.chunk.y + GetScreenHeight() / CHUNK_SIZE + 3;
        // std::cout << "Rendering chunk bounds from (" << sx << ", " << sy << ") with size (" << hx << ", " << hy << ")\n";

        // vertical lines
        for (int x = cam.chunk.x; x < hx; x++) {
            DrawLine((int)((x- cam.chunk.x)  * CHUNK_SIZE - cam.localPosition.x), (int)(0),
                     (int)((x- cam.chunk.x)  * CHUNK_SIZE - cam.localPosition.x), (int)(cy), color);
        }

        // horizontal lines
        for (int y = cam.chunk.y; y < hy; y++) {
            DrawLine((int)(0), (int)((y - cam.chunk.y) * CHUNK_SIZE - cam.localPosition.y),
                     (int)(cx), (int)((y - cam.chunk.y) * CHUNK_SIZE - cam.localPosition.y), color);
        }

    }

    // deconstructor
    void unloadAll() {
        colliderSystem.~ColliderSystem();
        kinematicsSystem.~KinematicsSystem();
        animationSystem.~AnimationSystem();

        TextureManager::unloadAllTextures();
    }
    
}
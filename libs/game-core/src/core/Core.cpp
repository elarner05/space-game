#include "core/Core.h"
#include "utils/debug_flags.h"
#include "input/InputHandler.h"
#include "components/EntityTypes.h"
#include "world/ChunkLoader.h"

#include <cassert>
// unhandled runtime assertion error, entityflags vector out of bounds operator[] access

namespace Core {
    AnimationSystem animationSystem;
    KinematicsSystem kinematicsSystem;
    ColliderSystem colliderSystem;
    RenderSystem renderSystem;
    GameCamera camera;

    ChunkLoader chunkLoader{"saves/"};
    SaveManager saveManager{"saves/"};

    robin_hood::unordered_map<ChunkCoord, std::vector<EntityID>> chunkMap;
    std::vector<EntityID> indexToEntity;
    std::vector<EntityTag> entityTagTable;
    std::vector<EntityFlags> entityFlagTable;
    std::vector<Slot> slots;        // grows to high watermark, never shrinks
    std::vector<uint32_t> freeList; // indices of available slots

    void init() {
        constexpr size_t expectedEntities = 1024; // reserve more memory as the number of entities grows, better allocation startegy

        kinematicsSystem.m_entities.reserve(expectedEntities);
        colliderSystem.m_original_entities.reserve(expectedEntities);
        colliderSystem.m_entities.reserve(expectedEntities);
        animationSystem.m_entities.reserve(expectedEntities);
        renderSystem.m_entities.reserve(expectedEntities);
        indexToEntity.reserve(expectedEntities);
        entityTagTable.reserve(expectedEntities);
        entityFlagTable.reserve(expectedEntities);
        slots.reserve(expectedEntities);
        freeList.reserve(expectedEntities);
    }

    void update(float dt) {
        
        kinematicsSystem.update(dt);
        colliderSystem.update(dt);

        chunkLoader.update();

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

        if ( Debug::showCameraPosition() ) {
            std::string text = "Chunk: " + std::to_string(camera.currentChunk.x) + " " + std::to_string(camera.currentChunk.y);

            int fontSize = 20;
            int padding = 10;

            int x = GetScreenWidth() - MeasureText(text.c_str(), fontSize) - padding;
            int y = GetScreenHeight() - fontSize - padding;

            DrawText(text.c_str(), x, y, fontSize, WHITE);
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
            kinematicsSystem.m_entities[slots[a.index].arrayIndex],
            kinematicsSystem.m_entities[slots[b.index].arrayIndex],
            colliderSystem.m_entities[slots[a.index].arrayIndex],
            colliderSystem.m_entities[slots[b.index].arrayIndex]
        );
    }



    // needs refactored into system
    void processCollisions(float dt) {
        ChunkCoord camChunk = camera.currentChunk;
        int simDist = GameCamera::simulationDistance;

        // track tested pairs to avoid duplicates across chunk neighbours
        robin_hood::unordered_set<uint64_t> testedPairs;

        auto pairKey = [](EntityID a, EntityID b) -> uint64_t {
            if (a > b) std::swap(a, b);
            return ((uint64_t)a.index << 32) | (uint64_t)b.index;
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
    EntityID registerEntity(EntityTag tag, Kinematics kin, CompoundCollider col, Animations anim, Texture2D tex, EntityFlags flags) {
        uint32_t slotIndex;
        if (!freeList.empty()) {
            slotIndex = freeList.back();
            freeList.pop_back();
        } else {
            slotIndex = static_cast<uint32_t>(slots.size());
            slots.push_back({0, 0});
        }

        uint32_t arrayIdx = static_cast<uint32_t>(kinematicsSystem.m_entities.size());
        EntityID id = EntityID{slotIndex, slots[slotIndex].generation};

        kinematicsSystem.registerEntity(kin);
        colliderSystem.registerEntity(col);
        animationSystem.registerEntity(anim);
        renderSystem.registerEntity(tex);
        entityTagTable.push_back(tag);
        entityFlagTable.push_back(flags);

        slots[slotIndex].arrayIndex = arrayIdx;
        indexToEntity.push_back(id);
        chunkMap[kin.chunk].push_back(id);
        kinematicsSystem.m_entities.back().computeAndSetBoundingRadius(colliderSystem.m_entities.back());
        return id;
    }

    // removes (deletes) an entity from all systems
    // maintains indirection table integrity for cache performance (does not reuse entityIDs)
    void unregisterEntity(EntityID id) {
        assert(id.isValid() && "unregistering invalid ID");
        assert(id.index < slots.size() && "ID never registered");
        assert(id.generation == slots[id.index].generation && "stale or double unregister");
        assert(entityFlagTable.size() == entityTagTable.size() && "flag/tag tables desynced");

        size_t idx = slots[id.index].arrayIndex;

        assert(idx < kinematicsSystem.m_entities.size() && "idx out of range");
        assert(idx < entityTagTable.size() && "entityTagTable out of range");
        assert(indexToEntity.size() == kinematicsSystem.m_entities.size() && "indexToEntity desynced");

        // remove from chunkMap before anything moves
        ChunkCoord chunk = kinematicsSystem.m_entities[idx].chunk;
        auto& list = chunkMap[chunk];
        list.erase(std::remove(list.begin(), list.end(), id), list.end());

        size_t lastIdx = kinematicsSystem.m_entities.size() - 1;
        EntityID lastEntity = indexToEntity[lastIdx];

        // swap components
        std::swap(kinematicsSystem.m_entities[idx],         kinematicsSystem.m_entities[lastIdx]);
        std::swap(animationSystem.m_entities[idx],          animationSystem.m_entities[lastIdx]);
        std::swap(renderSystem.m_entities[idx],             renderSystem.m_entities[lastIdx]);
        std::swap(colliderSystem.m_entities[idx],           colliderSystem.m_entities[lastIdx]);
        std::swap(colliderSystem.m_original_entities[idx],  colliderSystem.m_original_entities[lastIdx]);
        std::swap(entityTagTable[idx],                      entityTagTable[lastIdx]);
        std::swap(entityFlagTable[idx],                     entityFlagTable[lastIdx]);

        // fix slot for the entity that moved
        if (id != lastEntity) {
            slots[lastEntity.index].arrayIndex = static_cast<uint32_t>(idx);
            indexToEntity[idx] = lastEntity;
        }

        // invalidate slot and return to pool
        slots[id.index].generation++;
        slots[id.index].arrayIndex = UINT32_MAX;
        freeList.push_back(id.index);

        kinematicsSystem.m_entities.pop_back();
        animationSystem.m_entities.pop_back();
        renderSystem.m_entities.pop_back();
        colliderSystem.m_entities.pop_back();
        colliderSystem.m_original_entities.pop_back();
        entityTagTable.pop_back();
        entityFlagTable.pop_back();
        indexToEntity.pop_back();
    }

    // helpers to get components by ID, needs moved into systems
    Kinematics& getKinematics(EntityID id) {
        assert(id.generation == slots[id.index].generation && "stale EntityID");
        return kinematicsSystem.m_entities[slots[id.index].arrayIndex];
    }
    CompoundCollider& getCollider(EntityID id) {
        assert(id.generation == slots[id.index].generation && "stale EntityID");
        return colliderSystem.m_entities[slots[id.index].arrayIndex];
    }
    Animations& getAnimations(EntityID id) {
        assert(id.generation == slots[id.index].generation && "stale EntityID");
        return animationSystem.m_entities[slots[id.index].arrayIndex];
    }
    Texture2D& getTexture(EntityID id) {
        assert(id.generation == slots[id.index].generation && "stale EntityID");
        return renderSystem.m_entities[slots[id.index].arrayIndex];
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
        chunkLoader.saveAll();
        
        TextureManager::unloadAllTextures();
    }
    
}
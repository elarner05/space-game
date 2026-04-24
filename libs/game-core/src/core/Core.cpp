#include "core/Core.h"
#include "core/CollisionProcessor.h"

#include "utils/debug_flags.h"
#include "utils/TextureManager.h"

#include "input/InputHandler.h"

#include "components/Animation.h"
#include "components/Kinematics.h"
#include "components/CompoundCollider.h"
#include "components/EntityTypes.h"

#include "world/ChunkLoader.h"

#include "physics/gjk.h"
#include "physics/epa.h"
#include "utils/profiler.hpp"

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
        {
            ZoneScopedN("kinematics");
            kinematicsSystem.update(dt);
        }
        {
            ZoneScopedN("collider_rotation");
            colliderSystem.update(dt);

        }
        {
            ZoneScopedN("chunk_loading");
            chunkLoader.update();
        }

        {
            ZoneScopedN("collision_processing");
            processCollisions(dt);
        }


        camera.updatePosition(dt);
        {
            ZoneScopedN("animation");
            animationSystem.update(dt);
        }
    }

    // draw function
    void draw() {

        if ( Debug::showChunkBounds() ) {
            renderChunkBoundaries();
        }

        drawEntities();

        if ( Debug::showCameraPosition() ) {
            std::string text = "Chunk: " + std::to_string(camera.currentChunk.x) + " " + std::to_string(camera.currentChunk.y);

            int fontSize = 20;
            int padding = 10;

            int x = GetScreenWidth() - MeasureText(text.c_str(), fontSize) - padding;
            int y = GetScreenHeight() - fontSize - padding;

            DrawText(text.c_str(), x, y, fontSize, WHITE);
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

    // removes (deletes) an entity from all systems, swaps with the last entity to avoid holes, invalidates ID and returns slot to pool
    // could do with some cleanup
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
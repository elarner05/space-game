#include "core/Core.h"
#include "core/CollisionProcessor.h"
#include "core/Projectiles.h"
#include "core/ChunkMapUtil.h"

#include "core/KinematicsSystem.h"
#include "core/ColliderSystem.h"
#include "core/AnimationSystem.h"
#include "core/StatsSystem.h"

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

#include <iostream>
#include <cassert>
#include <cmath>

namespace Core {
    std::vector<Kinematics> kinematicsTable;
    std::vector<Animations> animationTable;
    std::vector<Texture2D> textureTable;
    std::vector<CompoundCollider> colliderTable;
    std::vector<CompoundCollider> originalColliderTable;
    std::vector<Stats> statsTable;
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

        kinematicsTable.reserve(expectedEntities);
        animationTable.reserve(expectedEntities);
        textureTable.reserve(expectedEntities);
        colliderTable.reserve(expectedEntities);
        originalColliderTable.reserve(expectedEntities);
        statsTable.reserve(expectedEntities);
        textureTable.reserve(expectedEntities);
        indexToEntity.reserve(expectedEntities);
        entityTagTable.reserve(expectedEntities);
        entityFlagTable.reserve(expectedEntities);
        slots.reserve(expectedEntities);
        freeList.reserve(expectedEntities);
    }

    // debug function
    void checkChunkMapValidity() {
        for (auto& [coord, vec] : chunkMap) {
            for (auto& id : vec) {
                assert(id.isValid() && "invalid EntityID in chunkMap");
                assert(id.index < slots.size() && "EntityID index out of range in chunkMap");
                std::cout << "ID 1: " << id.generation << "  ID 2: " << slots[id.index].generation << std::endl;

                assert(id.generation == slots[id.index].generation && "stale EntityID in chunkMap");
            }
        }
    }

    void update(float dt) {
        {
            ZoneScopedN("kinematics");
            Core::updateKinematics(dt);
        }
        
        {
            ZoneScopedN("collider_rotation");
            Core::updateColliders(dt);

        }

        {
            ZoneScopedN("collision_processing");
            processCollisions(dt);
            Core::resolveAllEntityChunks(); // chunks are not resolved until after collision processing
        }
        {  
            ZoneScopedN("projectiles");
            Core::Projectiles::update(dt);
        }
        {
            ZoneScopedN("stats");
            Core::updateStats(dt);
        }

        camera.updatePosition(dt);
        {
            ZoneScopedN("animation");
            Core::updateAnimations(dt);
        }
        
        {
            ZoneScopedN("chunk_loading");
            Core::chunkLoader.loadChunksInBuffer(); // load any chunks that are unloaded in which entities have just moved into
            Core::chunkLoader.update();
        }
        // checkChunkMapValidity(); debug
    }

    inline std::string to_1dp(float value) { // fast round float to string at 1dp for zoom level
        float rounded = std::round(value * 10.0f) / 10.0f;
        int whole = static_cast<int>(rounded);
        int frac = static_cast<int>(std::abs(rounded - whole) * 10 + 0.5f);
        return std::to_string(whole) + "." + std::to_string(frac);
    };

    // draw function
    void draw() {

        if ( Debug::showChunkBounds() ) {
            renderChunkBoundaries();
        }

        if ( Debug::showChunkLoadingBounds() ) {
            for (int dx = -camera.loadDistance; dx <= camera.loadDistance; dx++)
                for (int dy = -camera.loadDistance; dy <= camera.loadDistance; dy++)
                    colorChunk({camera.currentChunk.x + dx, camera.currentChunk.y + dy}, Color{150, 150, 150, 50});
        }
        
        Core::drawEntities();
        Core::Projectiles::renderProjectiles();
        // Core::Projectiles::debugRender();

        if ( Debug::showCameraPosition() ) {
            std::string text = "Chunk: " + std::to_string(camera.currentChunk.x) + " " + std::to_string(camera.currentChunk.y);

            int fontSize = 20;
            int padding = 10;

            int x = GetScreenWidth() - MeasureText(text.c_str(), fontSize) - padding;
            int y = GetScreenHeight() - fontSize - padding;

            DrawText(text.c_str(), x, y, fontSize, WHITE);
        }

        

        if (Debug::showDebugInfo()) {
            DrawFPS(10, 10);
            DrawText(("Entity count: " + std::to_string(kinematicsTable.size())).c_str(), 10, 50, 20, WHITE);
            DrawText(("Chunk count: " + std::to_string(chunkMap.size())).c_str(), 10, 80, 20, WHITE);
            DrawText(("Zoom level: " + to_1dp(camera.renderZoom) + "x").c_str(), 10, 110, 20, WHITE);
        }
    }
 
    // registers all the components of an entity to the system
    EntityID registerEntity(EntityTag tag, Kinematics kin, CompoundCollider col, Stats stats, Animations anim, Texture2D tex, EntityFlags flags) {
        uint32_t slotIndex;
        if (!freeList.empty()) {
            slotIndex = freeList.back();
            freeList.pop_back();
        } else {
            slotIndex = static_cast<uint32_t>(slots.size());
            slots.push_back({0, 0});
        }

        uint32_t arrayIdx = static_cast<uint32_t>(kinematicsTable.size());
        EntityID id = EntityID{slotIndex, slots[slotIndex].generation};

        kinematicsTable.emplace_back(kin);
        colliderTable.emplace_back(col);
        originalColliderTable.push_back(col); // force copy
        statsTable.emplace_back(stats);
        animationTable.emplace_back(anim);
        textureTable.emplace_back(tex);
        entityTagTable.emplace_back(tag);
        entityFlagTable.emplace_back(flags);

        slots[slotIndex].arrayIndex = arrayIdx;
        indexToEntity.emplace_back(id);
        
        chunkMap[kin.chunk].push_back(id);
        kinematicsTable.back().computeAndSetBoundingRadius(colliderTable.back());
        return id;
    }

    // removes (deletes) an entity from all systems, swaps with the last entity to avoid holes, invalidates ID and returns slot to pool
    // could do with some cleanup
    void unregisterEntity(EntityID id) {
        {
        assert(id.isValid() && "unregistering invalid ID");
        assert(id.index < slots.size() && "ID never registered");
        assert(id.generation == slots[id.index].generation && "stale or double unregister");
        assert(kinematicsTable.size() == colliderTable.size() &&
               kinematicsTable.size() == originalColliderTable.size() &&
               kinematicsTable.size() == animationTable.size() &&
               kinematicsTable.size() == textureTable.size() &&
               kinematicsTable.size() == entityTagTable.size() &&
               kinematicsTable.size() == entityFlagTable.size() &&
               "component/entity tables desynced");
        assert(entityFlagTable.size() == entityTagTable.size() && "flag/tag tables desynced");
        }
        size_t idx = slots[id.index].arrayIndex;
        {
        assert(idx < kinematicsTable.size() && "idx out of range");
        assert(idx < entityTagTable.size() && "entityTagTable out of range");
        assert(indexToEntity.size() == kinematicsTable.size() && "indexToEntity desynced");
        }
        // remove from chunkMap before anything moves
        ChunkCoord& chunk = kinematicsTable[idx].chunk;
 
        auto it = chunkMap.find(chunk);
        assert(it != chunkMap.end() && "chunk not found in chunkMap");

        std::vector<EntityID>& list = it->second;
        
        assert(std::find(list.begin(), list.end(), id) != list.end() && "entity not found in chunkMap");
        list.erase(std::remove(list.begin(), list.end(), id), list.end());

        size_t lastIdx = kinematicsTable.size() - 1;
        EntityID lastEntity = indexToEntity[lastIdx];

        // swap components
        std::swap(kinematicsTable[idx],       kinematicsTable[lastIdx]);
        std::swap(animationTable[idx],        animationTable[lastIdx]);
        std::swap(textureTable[idx],          textureTable[lastIdx]);
        std::swap(colliderTable[idx],         colliderTable[lastIdx]);
        std::swap(originalColliderTable[idx], originalColliderTable[lastIdx]);
        std::swap(statsTable[idx],            statsTable[lastIdx]);
        std::swap(entityTagTable[idx],        entityTagTable[lastIdx]);
        std::swap(entityFlagTable[idx],       entityFlagTable[lastIdx]);

        // fix slot for the entity that moved
        if (id != lastEntity) {
            slots[lastEntity.index].arrayIndex = static_cast<uint32_t>(idx);
            indexToEntity[idx] = lastEntity;
        }

        // invalidate slot and return to pool
        slots[id.index].generation++;
        slots[id.index].arrayIndex = UINT32_MAX;
        freeList.push_back(id.index);

        kinematicsTable.pop_back();
        animationTable.pop_back();
        textureTable.pop_back();
        colliderTable.pop_back();
        statsTable.pop_back();
        originalColliderTable.pop_back();
        entityTagTable.pop_back();
        entityFlagTable.pop_back();
        indexToEntity.pop_back();
    }

    // helpers to get components by ID, needs moved into systems
    Kinematics& getKinematics(EntityID id) {
        assert(id.generation == slots[id.index].generation && "stale EntityID");
        return kinematicsTable[slots[id.index].arrayIndex];
    }
    CompoundCollider& getCollider(EntityID id) {
        assert(id.generation == slots[id.index].generation && "stale EntityID");
        return colliderTable[slots[id.index].arrayIndex];
    }
    Animations& getAnimations(EntityID id) {
        assert(id.generation == slots[id.index].generation && "stale EntityID");
        return animationTable[slots[id.index].arrayIndex];
    }
    Texture2D& getTexture(EntityID id) {
        assert(id.generation == slots[id.index].generation && "stale EntityID");
        return textureTable[slots[id.index].arrayIndex];
    }

    inline Vector2 worldToScreen(Vector2 worldOffset) {
        // worldOffset = world-pixel distance from camera center
        return Vector2{
            worldOffset.x * camera.renderZoom + GetScreenWidth()  * 0.5f,
            worldOffset.y * camera.renderZoom + GetScreenHeight() * 0.5f
        };
    }

    inline Vector2 chunkToWorld(ChunkCoord coord) {
        // world-pixel offset from camera center to top-left of chunk
        const ChunkCoord& camChunk = Core::camera.currentChunk;
        const Vector2&    camPos   = Core::camera.kinematics.localPosition;
        return Vector2{
            (float)(coord.x - camChunk.x) * CHUNK_SIZEF - camPos.x,
            (float)(coord.y - camChunk.y) * CHUNK_SIZEF - camPos.y
        };
    }

    void colorChunk(ChunkCoord coord, Color color) {
        float zoom = Core::camera.renderZoom;
        Vector2 screenPos = worldToScreen(chunkToWorld(coord));
        DrawRectangle(screenPos.x, screenPos.y,
            (int)(CHUNK_SIZE * zoom), (int)(CHUNK_SIZE * zoom), color);
    }

    void renderChunkBoundaries(Color color) {
        float cx = GetScreenWidth();
        float cy = GetScreenHeight();
        float zoom = Core::camera.renderZoom;
        const ChunkCoord& camChunk = Core::camera.currentChunk;

        const int sx = camChunk.x - (int)(cx / (CHUNK_SIZE * zoom) * 0.5f) - 1;
        const int sy = camChunk.y - (int)(cy / (CHUNK_SIZE * zoom) * 0.5f) - 1;
        const int hx = camChunk.x + (int)(cx / (CHUNK_SIZE * zoom) * 0.5f) + 2;
        const int hy = camChunk.y + (int)(cy / (CHUNK_SIZE * zoom) * 0.5f) + 2;

        for (int x = sx; x < hx; x++) {
            Vector2 screenPos = worldToScreen(chunkToWorld({x, 0}));
            if (screenPos.x < 0.f || screenPos.x > cx) continue;
            DrawLine((int)screenPos.x, 0, (int)screenPos.x, (int)cy, color);
        }
        for (int y = sy; y < hy; y++) {
            Vector2 screenPos = worldToScreen(chunkToWorld({0, y}));
            if (screenPos.y < 0.f || screenPos.y > cy) continue;
            DrawLine(0, (int)screenPos.y, (int)cx, (int)screenPos.y, color);
        }
    }

    // deconstructor
    void unloadAll() {
        chunkLoader.saveAll();
        
        TextureManager::unloadAllTextures();
    }    
}

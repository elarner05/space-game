#pragma once

// need to move as many imports to the cpp as possible for encapsulation
#include "raylib.h"

#include "core/System.h"
#include "core/ColliderSystem.h"
#include "core/KinematicsSystem.h"
#include "core/AnimationSystem.h"
#include "core/RenderSystem.h"

#include "robin_hood.h"

#include "world/GameCamera.h"
#include "core/EntityID.h"
#include "components/EntityTypes.h"
#include "world/ChunkLoader.h"
#include "world/SaveManager.h"

namespace Core {

    // global game state, needs to be made more encapsulated
    extern GameCamera camera;
    extern robin_hood::unordered_map<ChunkCoord, std::vector<EntityID>> chunkMap;
    extern std::vector<EntityID> indexToEntity;
    extern std::vector<EntityTag> entityTagTable;
    extern std::vector<EntityFlags> entityFlagTable;
    extern std::vector<Slot> slots;
    extern std::vector<uint32_t> freeList; // indices of available slots

    extern AnimationSystem animationSystem;
    extern KinematicsSystem kinematicsSystem;
    extern ColliderSystem colliderSystem;
    extern RenderSystem renderSystem;

    extern ChunkLoader chunkLoader;
    extern SaveManager saveManager;


    void init();

    void update(float dt);
    void draw();

    EntityID registerEntity(EntityTag tag, Kinematics kin, CompoundCollider col, Animations anim, Texture2D tex, EntityFlags flags=EntityFlags::None);
    void unregisterEntity(EntityID id);

    Kinematics& getKinematics(EntityID id);
    CompoundCollider& getCollider(EntityID id);
    Animations& getAnimations(EntityID id);
    Texture2D& getTexture(EntityID id);
    
    void renderChunkBoundaries(Color color = GRAY);
    void colorChunk(ChunkCoord coord, Color color = GRAY);

    void unloadAll();
    
}
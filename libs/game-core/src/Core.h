#pragma once

// need to move as many imports to the cpp as possible for encapsulation
#include "raylib.h"
#include "Animation.h"
#include "Kinematics.h"
#include "CompoundCollider.h"

#include "System.h"
#include "ColliderSystem.h"
#include "KinematicsSystem.h"
#include "AnimationSystem.h"
#include "RenderSystem.h"

#include "gjk.h"
#include "epa.h"
#include "robin_hood.h"

#include "TextureManager.h"
#include "GameCamera.h"
#include "EntityID.h"
namespace Core {
    extern GameCamera camera;
    extern robin_hood::unordered_map<ChunkCoord, std::vector<EntityID>> chunkMap;

    void init();

    void update(float dt);
    void draw();
    void processCollisions(float dt);

    EntityID registerEntity(Kinematics kin, CompoundCollider col, Animations anim, Texture2D tex);
    void unregisterEntity(EntityID id);

    Kinematics& getKinematics(EntityID id);
    CompoundCollider& getCollider(EntityID id);
    Animations& getAnimations(EntityID id);
    Texture2D& getTexture(EntityID id);
    
    void renderChunkBoundaries(Color color = GRAY);

    void unloadAll();
    
}
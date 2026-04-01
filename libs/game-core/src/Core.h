#pragma once

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

#include "TextureManager.h"
#include "GameCamera.h"

namespace Core {
    extern GameCamera camera;

    void init();

    void update(float dt);
    void draw();
    void processCollisions(float dt);
    // bool handleCollision(Kinematics& kinA, Kinematics& kinB, CompoundCollider& colA, CompoundCollider& colB);

    CompoundCollider* registerComponent(CompoundCollider component);
    Kinematics* registerComponent(Kinematics component);
    Animations* registerComponent(Animations component);
    Texture2D* registerComponent(Texture2D component);
    
    void renderChunkBoundaries(Color color = GRAY);

    void unloadAll();
    
}
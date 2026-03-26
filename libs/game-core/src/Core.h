#pragma once

#include "raylib.h"
#include "Animation.h"
#include "Kinematics.h"
#include "CompoundCollider.h"

#include "System.h"
#include "ColliderSystem.h"
#include "KinematicsSystem.h"
#include "AnimationSystem.h"

#include "TextureManager.h"

namespace Core {
    // AnimationSystem animationSystem;
    // KinematicsSystem kinematicsSystem;
    // ColliderSystem colliderSystem;

    void init();

    void update(float dt);

    CompoundCollider* registerComponent(CompoundCollider component);
    Kinematics* registerComponent(Kinematics component);
    Animations* registerComponent(Animations component);
    

    void unloadAll();
    
}
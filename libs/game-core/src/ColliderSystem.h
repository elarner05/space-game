#pragma once
#include "System.h"
#include "CompoundCollider.h"
#include "raymath.h"
#include "raylib.h"  // for TraceLog
#include <deque>

class ColliderSystem : public System<CompoundCollider> {
protected:
    std::deque<CompoundCollider> m_original_entities;
public:
    ColliderSystem();
    ~ColliderSystem();

    void update(float dt) override;
    CompoundCollider* registerEntity(CompoundCollider entity) override;

    bool checkNeedsRotationUpdate();
    void applyRotations();
    
    static void TestRotation();
};

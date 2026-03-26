#pragma once
#include "System.h"
#include "CompoundCollider.h"
#include "raymath.h"
#include "raylib.h"  // for TraceLog
#include <deque>

class ColliderSystem : public System<CompoundCollider> {
protected:
    std::deque<CompoundCollider> m_original_entities;
    std::deque<Vector2> m_positions; // parallel deque to m_entities to store positions of colliders for collision checks
public:
    ColliderSystem();
    ~ColliderSystem();

    void update(float dt) override;
    CompoundCollider* registerEntity(CompoundCollider entity) override;

    void syncPositions(const std::deque<Vector2>& updatedPos);
    bool checkNeedsRotationUpdate();
    void applyRotations();
    
    static void TestRotation();
};

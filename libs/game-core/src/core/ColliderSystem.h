#pragma once
#include "core/System.h"
#include "components/CompoundCollider.h"
#include "raymath.h"
#include "raylib.h"  // for TraceLog
#include <vector>

class ColliderSystem : public System<CompoundCollider> {
public:
    std::vector<CompoundCollider> m_original_entities;
public:
    ColliderSystem();
    ~ColliderSystem();

    void update(float dt) override;
    void registerEntity(CompoundCollider entity) override;

    bool checkNeedsRotationUpdate();
    void applyRotations();
    
    static void TestRotation();
};

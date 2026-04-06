#pragma once
#include "System.h"
#include "CompoundCollider.h"
#include "raymath.h"
#include "raylib.h"  // for TraceLog
#include <vector>

class ColliderSystem : public System<CompoundCollider> {
protected:
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

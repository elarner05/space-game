#pragma once
#include "physics/gjk.h"
#include "physics/epa.h"
#include "core/EntityID.h"
struct PendingCollision {
    EntityID      idA, idB;
    ContactManifold manifold;
};
namespace Core {
    void processCollisions(float dt);
}
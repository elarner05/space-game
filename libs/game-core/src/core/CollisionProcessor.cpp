#include "core/CollisionProcessor.h"
#include "physics/gjk.h"
#include "physics/epa.h"
#include "components/Kinematics.h"
#include "components/CompoundCollider.h"
#include "core/Core.h"

#include "utils/profiler.hpp"
// #include "utils/debug_flags.h"

namespace Core {
    bool handleCollision(Kinematics& kinA, Kinematics& kinB, CompoundCollider& colA, CompoundCollider& colB){
        bool handled = false;
        constexpr float elasticity = 1.f;

        // Express both entities relative to the camera's position
        Vector2 posA = kinA.localPositionRelativeTo(camera.kinematics.chunk, camera.kinematics.localPosition);
        Vector2 posB = kinB.localPositionRelativeTo(camera.kinematics.chunk, camera.kinematics.localPosition);

        for (int i = 0; i < colA.colliderCount; i++) {
            for (int j = 0; j < colB.colliderCount; j++) {
                if (!withinBounds(colA.colliders[i], posA, colB.colliders[j], posB))
                    continue;

                ContactManifold manifold = {};

                if (colA.colliders[i].count == 0 && colB.colliders[j].count == 0) {
                    // Circle-circle: bypass GJK/EPA
                    const Collider& cA = colA.colliders[i];
                    const Collider& cB = colB.colliders[j];
                    Vector2 centerA = Vector2Add(posA, cA.offset);
                    Vector2 centerB = Vector2Add(posB, cB.offset);
                    Vector2 delta   = Vector2Subtract(centerA, centerB);
                    float   dist    = Vector2Length(delta);
                    manifold.normal  = (dist > 1e-6f) ? Vector2Normalize(delta) : Vector2{ 1.f, 0.f };
                    manifold.depth   = (cA.radius + cB.radius) - dist;
                    manifold.contact = Vector2Add(centerB, Vector2Scale(manifold.normal, cB.radius));
                    manifold.valid   = manifold.depth > 0.f;
                } else {
                    // Polygon or mixed: GJK + EPA
                    Vector2 simplex[3] = {};
                    if (!gjk(colA.colliders[i], posA, colB.colliders[j], posB, simplex))
                        continue;
                    manifold = epa(colA.colliders[i], posA, colB.colliders[j], posB, simplex);
                }

                if (!manifold.valid) continue;

                resolveCollision(manifold, &kinA, posA, &kinB, posB, elasticity);
                positionalCorrection(manifold, &kinA, &kinB);
                handled = true;
            }
        }
        return handled;
    }

    // helper to call by ID instead of components
    void handleCollision(EntityID a, EntityID b) {
        handleCollision(
            kinematicsSystem.m_entities[slots[a.index].arrayIndex],
            kinematicsSystem.m_entities[slots[b.index].arrayIndex],
            colliderSystem.m_entities[slots[a.index].arrayIndex],
            colliderSystem.m_entities[slots[b.index].arrayIndex]
        );
    }

    void processCollisions(float dt) {
        
        ChunkCoord camChunk = camera.currentChunk;
        int simDist = GameCamera::simulationDistance;

        // track tested pairs to avoid duplicates across chunk neighbours
        robin_hood::unordered_set<uint64_t> testedPairs;

        auto pairKey = [](EntityID a, EntityID b) -> uint64_t {
            if (a > b) std::swap(a, b);
            return ((uint64_t)a.index << 32) | (uint64_t)b.index;
        };
        for (int dx = -simDist; dx <= simDist; dx++) {
            for (int dy = -simDist; dy <= simDist; dy++) {
                ZoneScopedN("collision_chunk");
                ChunkCoord chunk = { camChunk.x + dx, camChunk.y + dy };
                auto it = chunkMap.find(chunk);
                if (it == chunkMap.end()) continue;

                const std::vector<EntityID>& locals = it->second;

                // test within this chunk
                for (size_t i = 0; i < locals.size(); i++) {
                    for (size_t j = i + 1; j < locals.size(); j++) {
                        EntityID a = locals[i], b = locals[j];
                        if (testedPairs.insert(pairKey(a, b)).second)
                            handleCollision(a, b);
                    }
                }

                // test against each neighbour chunk
                for (int nx = -1; nx <= 1; nx++) {
                    for (int ny = -1; ny <= 1; ny++) {
                        if (nx == 0 && ny == 0) continue;
                        ChunkCoord neighbour = { chunk.x + nx, chunk.y + ny };
                        auto nit = chunkMap.find(neighbour);
                        if (nit == chunkMap.end()) continue;

                        for (EntityID a : locals) {
                            for (EntityID b : nit->second) {
                                if (testedPairs.insert(pairKey(a, b)).second)
                                    handleCollision(a, b);
                            }
                        }
                    }
                }
            }
        }
    }
}
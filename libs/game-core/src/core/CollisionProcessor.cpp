#include "core/CollisionProcessor.h"
#include "physics/gjk.h"
#include "physics/epa.h"
#include "components/Kinematics.h"
#include "components/CompoundCollider.h"
#include "core/Core.h"
#include "core/ChunkMapUtil.h"

#include "utils/profiler.hpp"
// #include "utils/debug_flags.h"

namespace Core {
    bool handleCollision(const EntityID& idA, const EntityID& idB, Kinematics& kinA, Kinematics& kinB, CompoundCollider& colA, CompoundCollider& colB){
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
                // chunk resolved externally, chunkmap is being looped over
                handled = true;
            }
        }
        return handled;
    }

    void processCollisions(float dt) {
        
        ChunkCoord camChunk = camera.currentChunk;
        int simDist = GameCamera::simulationDistance;

        for (int dx = -simDist; dx <= simDist; dx++) {
            for (int dy = -simDist; dy <= simDist; dy++) {
                ZoneScopedN("collision_chunk");
                ChunkCoord chunk = { camChunk.x + dx, camChunk.y + dy };
                auto it = Core::chunkMap.find(chunk);
                if (it == Core::chunkMap.end()) continue;
                const auto& locals = it->second;

                // Within-chunk pairs
                for (size_t i = 0; i < locals.size(); i++) {
                    size_t idxA = slots[locals[i].index].arrayIndex;
                    Kinematics& kinA = kinematicsSystem.m_entities[idxA];
                    CompoundCollider& colA = colliderSystem.m_entities[idxA];
                    for (size_t j = i + 1; j < locals.size(); j++) {
                        size_t idxB = slots[locals[j].index].arrayIndex;
                        handleCollision(locals[i], locals[j], kinA, kinematicsSystem.m_entities[idxB],
                                        colA, colliderSystem.m_entities[idxB]);
                    }
                }

                // Forward neighbours only to avoid duplicate cross-chunk pairs
                constexpr std::pair<int,int> forwardNeighbours[] = {
                    {1,0},{-1,1},{0,1},{1,1}
                };
                for (auto [nx, ny] : forwardNeighbours) {
                    ChunkCoord neighbour = { chunk.x + nx, chunk.y + ny };
                    auto nit = Core::chunkMap.find(neighbour);
                    if (nit == Core::chunkMap.end()) continue;
                    for (EntityID a : locals) {
                        size_t idxA = slots[a.index].arrayIndex;
                        Kinematics& kinA = kinematicsSystem.m_entities[idxA];
                        CompoundCollider& colA = colliderSystem.m_entities[idxA];
                        for (EntityID b : nit->second) {
                            size_t idxB = slots[b.index].arrayIndex;
                            handleCollision(a, b, kinA, kinematicsSystem.m_entities[idxB],
                                            colA, colliderSystem.m_entities[idxB]);
                        }
                    }
                }
            }
        }

    }
}
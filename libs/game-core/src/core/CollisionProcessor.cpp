#include "core/CollisionProcessor.h"
#include "utils/ThreadPool.h"
#include "physics/gjk.h"
#include "physics/epa.h"
#include "components/Kinematics.h"
#include "components/CompoundCollider.h"
#include "core/Core.h"
#include "core/ChunkMapUtil.h"
#include "utils/profiler.hpp"

#include <vector>
#include <mutex>
#include <future>

namespace Core {

    // detection only, no writes to kinematics; multi-threaded path
    void detectCollisions(const EntityID& idA, const EntityID& idB,
                          const Kinematics& kinA, const Kinematics& kinB,
                          const CompoundCollider& colA, const CompoundCollider& colB,
                          std::vector<PendingCollision>& out) {

        Vector2 posA = kinA.localPositionRelativeTo(camera.kinematics.chunk, camera.kinematics.localPosition);
        Vector2 posB = kinB.localPositionRelativeTo(camera.kinematics.chunk, camera.kinematics.localPosition);

        for (int i = 0; i < colA.colliderCount; i++) {
            for (int j = 0; j < colB.colliderCount; j++) {
                if (!withinBounds(colA.colliders[i], posA, colB.colliders[j], posB))
                    continue;

                ContactManifold manifold = {};

                if (colA.colliders[i].count == 0 && colB.colliders[j].count == 0) {
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
                    Vector2 simplex[3] = {};
                    if (!gjk(colA.colliders[i], posA, colB.colliders[j], posB, simplex))
                        continue;
                    manifold = epa(colA.colliders[i], posA, colB.colliders[j], posB, simplex);
                }

                if (!manifold.valid) continue;
                out.push_back({ idA, idB, manifold });
            }
        }
    }

    // resolve all the collisions in the pending list, single threaded path
    void resolveAll(const std::vector<PendingCollision>& pending) {
        constexpr float elasticity = 1.f;
        for (const auto& pc : pending) {
            size_t idxA = slots[pc.idA.index].arrayIndex;
            size_t idxB = slots[pc.idB.index].arrayIndex;
            Kinematics& kinA = kinematicsTable[idxA];
            Kinematics& kinB = kinematicsTable[idxB];

            // Recompute positions for response — kinematics unchanged during detection
            Vector2 posA = kinA.localPositionRelativeTo(camera.kinematics.chunk, camera.kinematics.localPosition);
            Vector2 posB = kinB.localPositionRelativeTo(camera.kinematics.chunk, camera.kinematics.localPosition);

            resolveCollision(pc.manifold, &kinA, posA, &kinB, posB, elasticity);
            positionalCorrection(pc.manifold, &kinA, &kinB);
        }
    }

void processCollisions(float dt) {
    ChunkCoord camChunk = camera.currentChunk;
    int simDist = GameCamera::simulationDistance;

    // Phase 1: Collect all pairs to test (single-threaded)
    struct EntityPair {
        EntityID idA, idB;
        size_t   idxA, idxB;
    };
    std::vector<EntityPair> pairs;
    pairs.reserve(512);

    constexpr std::pair<int,int> forwardNeighbours[] = {
        {1,0},{-1,1},{0,1},{1,1}
    };

    for (int dx = -simDist; dx <= simDist; dx++) {
        for (int dy = -simDist; dy <= simDist; dy++) {
            ChunkCoord chunk = { camChunk.x + dx, camChunk.y + dy };
            auto it = Core::chunkMap.find(chunk);
            if (it == Core::chunkMap.end()) continue;
            const auto& locals = it->second;

            // within-chunk
            for (size_t i = 0; i < locals.size(); i++) {
                size_t idxA = slots[locals[i].index].arrayIndex;
                for (size_t j = i + 1; j < locals.size(); j++) {
                    size_t idxB = slots[locals[j].index].arrayIndex;
                    pairs.push_back({ locals[i], locals[j], idxA, idxB });
                }
            }

            // cross-chunk forward neighbours
            for (auto [nx, ny] : forwardNeighbours) {
                ChunkCoord neighbour = { chunk.x + nx, chunk.y + ny };
                auto nit = Core::chunkMap.find(neighbour);
                if (nit == Core::chunkMap.end()) continue;
                for (EntityID a : locals) {
                    size_t idxA = slots[a.index].arrayIndex;
                    for (EntityID b : nit->second) {
                        size_t idxB = slots[b.index].arrayIndex;
                        pairs.push_back({ a, b, idxA, idxB });
                    }
                }
            }
        }
    }

    if (pairs.empty()) return;

    // Phase 2: Detect across all pairs, split into even batches (multi-threaded)
    const size_t threadCount = gThreadPool.size();  // add a size() getter if not present
    const size_t batchSize   = std::max(size_t(1), (pairs.size() + threadCount - 1) / threadCount);

    std::vector<std::vector<PendingCollision>> perThreadResults(threadCount);
    std::vector<std::future<void>>             futures;
    futures.reserve(threadCount);

    for (size_t t = 0; t < threadCount; t++) {
        size_t begin = t * batchSize;
        if (begin >= pairs.size()) break;
        size_t end = std::min(begin + batchSize, pairs.size());

        futures.push_back(gThreadPool.submit([&, t, begin, end] {
            auto& out = perThreadResults[t];
            for (size_t k = begin; k < end; k++) {
                const auto& p = pairs[k];
                detectCollisions(p.idA, p.idB,
                                 kinematicsTable[p.idxA], kinematicsTable[p.idxB],
                                 colliderTable[p.idxA],   colliderTable[p.idxB],
                                 out);
            }
        }));
    }

    for (auto& f : futures) f.get();

    // Phase 3: Resolve (single-threaded)
    for (auto& v : perThreadResults)
        resolveAll(v);
}
}
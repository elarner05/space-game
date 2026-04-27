#include "core/ChunkMapUtil.h"
#include <cassert>

bool Core::resolveEntityChunk(Kinematics& kin, EntityID id) {
    ChunkCoord oldChunk = kin.chunk;

    // mutate chunk if needed
    if (!kin.resolveChunk())
        return false;

    ChunkCoord newChunk = kin.chunk;

    // remove from old chunk
    auto it = Core::chunkMap.find(oldChunk);
    if (it == Core::chunkMap.end()) {
        assert(false && "oldChunk missing in chunkMap");
    }

    auto& oldList = it->second;

    auto it2 = std::find(oldList.begin(), oldList.end(), id);
    if (it2 == oldList.end()) {
        printf("ERROR: entity not in expected oldChunk\n");
        printf("  id.index=%u gen=%u\n", id.index, id.generation);
        printf("  oldChunk = {%d,%d}\n", oldChunk.x, oldChunk.y);

        // scan all chunks (debug nuclear option)
        for (auto& [coord, vec] : Core::chunkMap) {
            for (auto& e : vec) {
                if (e.index == id.index) {
                    printf("  FOUND in chunk {%d,%d} gen=%u\n",
                           coord.x, coord.y, e.generation);
                }
            }
        }

        assert(false && "Entity not found in expected old chunk");
    }

    // swap-and-pop removal
    *it2 = oldList.back();
    oldList.pop_back();

    // insert into new chunk
    auto& newList = Core::chunkMap[newChunk];

    // debug: prevent duplicates
    // assert(std::find(newList.begin(), newList.end(), id) == newList.end()
        //    && "Entity already exists in new chunk");

    newList.push_back(id);

    // ensure chunk is loaded
    Core::chunkLoader.addToChunkBuffer(newChunk);

    return true;
}

// could add flag to kinematics for when the entity has moved, so its O(M) instead of O(N) [M = moved entities]
bool Core::resolveAllEntityChunks() {
    for (size_t i = 0; i < Core::kinematicsSystem.m_entities.size(); ++i) {
        Kinematics& kin = Core::kinematicsSystem.m_entities[i];
        EntityID id = Core::indexToEntity[i];
        Core::resolveEntityChunk(kin, id);
    }
    return true;
}
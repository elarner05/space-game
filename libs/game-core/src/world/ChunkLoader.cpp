#include "world/ChunkLoader.h"
#include "components/EntityTypes.h"
#include "core/Core.h"
#include "core/EntityFactory.h"

#include <climits>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>

void ChunkLoader::update() {
    ChunkCoord camChunk = Core::camera.currentChunk;

    // Only update when camera crosses chunk boundary; needs to account for loading chunks to move entities into the correct chunk
    // currently fast enough to constantly check
    // if (camChunk == m_lastCameraChunk)
    //     return;
    if (!needsUpdate) return;

    // m_lastCameraChunk = camChunk;

    const int ld = GameCamera::loadDistance;

    robin_hood::unordered_set<ChunkCoord> desiredChunks;
    desiredChunks.reserve((2 * ld + 1) * (2 * ld + 1));

    // chunks that should be loaded, around camera
    for (int dx = -ld; dx <= ld; dx++) {
        for (int dy = -ld; dy <= ld; dy++) {
            desiredChunks.insert({ camChunk.x + dx, camChunk.y + dy });
        }
    }

    std::vector<ChunkCoord> toLoad;
    std::vector<ChunkCoord> toUnload;

    toLoad.reserve(desiredChunks.size());
    toUnload.reserve(m_loadedChunks.size());

    // needs loaded
    for (const ChunkCoord& c : desiredChunks) {
        if (!m_loadedChunks.contains(c)) {
            toLoad.push_back(c);
        }
    }

    // needs unloaded
    for (const ChunkCoord& c : m_loadedChunks) {
        if (!desiredChunks.contains(c)) {
            toUnload.push_back(c);
        }
    }

    // load
    for (const ChunkCoord& c : toLoad) {
        loadChunk(c);
        m_loadedChunks.insert(c);
    }

    // unload
    for (const ChunkCoord& c : toUnload) {
        unloadChunk(c);
        m_loadedChunks.erase(c);
    }

    needsUpdate = false;
}

// File layout per chunk:
// [uint32] versionNumber
// [uint32] entityCount
// per entity:
//   [uint8]  entityTypeID     (index into a registry of EntityTypes)
//   Kinematics                (plain memcpy)



ChunkLoader::ChunkLoader(const std::string& savePath)
    : m_chunkBuffer()
    , m_savePath(savePath)
    , needsUpdate(true)
    // , m_lastCameraChunk({INT_MIN, INT_MIN}) // forces first update to load
{
    m_chunkBuffer.reserve((GameCamera::loadDistance * 2 + 1) * (GameCamera::loadDistance * 2 + 1));
    std::filesystem::create_directories(savePath);
}



static std::string chunkPath(const std::string& savePath, ChunkCoord coord) {
    return savePath + "/" + std::to_string(coord.x) + "_" + 
                            std::to_string(coord.y) + ".bin";
}


void ChunkLoader::loadChunk(ChunkCoord coord) {
    std::string path = chunkPath(m_savePath, coord);
    if (!std::filesystem::exists(path)) return;

    std::ifstream file(path, std::ios::binary);
    if (!file) return;

    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != ChunkLoader::CHUNK_FORMAT_VERSION) return; // silently skip incompatible saves

    deserializeChunk(coord, file);
}

// loads a chunk if not loaded, used for when an entity moves into an unloaded chunk to perserve the chunks integrity
void ChunkLoader::loadChunkIfUnloaded(ChunkCoord coord) {
    if (chunkIsLoaded(coord)) {
        return;
    }
    loadChunk(coord);
    m_loadedChunks.insert(coord);
}

void ChunkLoader::unloadChunk(ChunkCoord coord) {
    auto it = Core::chunkMap.find(coord);
    if (it == Core::chunkMap.end()) return;

    if (it->second.empty()) {
        deleteChunkFile(coord);
        Core::chunkMap.erase(coord);
        return;
    }

    std::string path = chunkPath(m_savePath, coord);

    std::vector<EntityID> toUnregister;
    toUnregister.reserve(20);
    uint32_t count = 0;

    std::vector<EntityID> snapshot(it->second.begin(), it->second.end());
    for (EntityID id : snapshot) {
        // assert(std::count(it->second.begin(), it->second.end(), id) == 1 
        //    && "duplicate EntityID in chunkMap");
        size_t idx = Core::slots[id.index].arrayIndex;
        if ((Core::entityFlagTable[idx] & EntityFlags::Persistent) == EntityFlags::None) {
            count++;
            toUnregister.push_back(id);
        }
    }

    if (count == 0) {
        deleteChunkFile(coord);
        it->second.clear();
        Core::chunkMap.erase(coord);
        return;
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        TraceLog(LOG_WARNING, "File could not be opened for chunk!");
        it->second.clear();
        Core::chunkMap.erase(coord);
        return;
    }

    file.write(reinterpret_cast<const char*>(&ChunkLoader::CHUNK_FORMAT_VERSION),
               sizeof(ChunkLoader::CHUNK_FORMAT_VERSION));
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (EntityID id : toUnregister) {
        size_t idx = Core::slots[id.index].arrayIndex;
        EntityTag tag = Core::entityTagTable[idx];
        file.write(reinterpret_cast<const char*>(&tag), sizeof(EntityTag));
        const Kinematics& kin = Core::kinematicsSystem.m_entities[idx];
        assert(kin.chunk == coord);
        file.write(reinterpret_cast<const char*>(&kin), sizeof(Kinematics));
    }

    // Sort toUnregister by arrayIndex descending before the unregister loop
    std::sort(toUnregister.begin(), toUnregister.end(), [](EntityID a, EntityID b) {
        return Core::slots[a.index].arrayIndex > Core::slots[b.index].arrayIndex;
    });

    for (EntityID id : toUnregister)
        Core::unregisterEntity(id);  // chunkMap entry removed inside here
    
    if (Core::chunkMap.count(coord) && Core::chunkMap[coord].empty()) {
        Core::chunkMap.erase(coord);
    }
    // for (auto& [chunk, list] : Core::chunkMap) {   debugging to check chunkmap integrity
    //     for (auto id : list) {
    //         size_t idx = Core::slots[id.index].arrayIndex;
    //         auto& kin = Core::kinematicsSystem.m_entities[idx];
    //         assert(kin.chunk == chunk && "Post-unload mismatch");
    //     }
    // }

}

// allows batching chunk loads/unloads
void ChunkLoader::addToChunkBuffer(ChunkCoord coord) {
    m_chunkBuffer.insert(coord);
}
void ChunkLoader::removeFromChunkBuffer(ChunkCoord coord) {
    m_chunkBuffer.erase(coord);
}

void ChunkLoader::loadChunksInBuffer() {
    for (const ChunkCoord& coord : m_chunkBuffer) {
        if (!chunkIsLoaded(coord)) {
            loadChunk(coord);
            m_loadedChunks.insert(coord);
        }
    }
    m_chunkBuffer.clear();

    needsUpdate = true;
}

void ChunkLoader::unloadChunksInBuffer() {
    for (const ChunkCoord& coord : m_chunkBuffer) {
        if (chunkIsLoaded(coord)) {
            unloadChunk(coord);
            m_loadedChunks.erase(coord);
        }
    }
    m_chunkBuffer.clear();
}

void ChunkLoader::deleteChunkFile(ChunkCoord coord) {
    std::string path = chunkPath(m_savePath, coord);
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
}

bool ChunkLoader::deserializeChunk(ChunkCoord coord, std::ifstream& file) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (uint32_t i = 0; i < count; i++) {
        EntityTag tag{};
        file.read(reinterpret_cast<char*>(&tag), sizeof(EntityTag));

        Kinematics kin{};
        file.read(reinterpret_cast<char*>(&kin), sizeof(Kinematics));
        assert(kin.chunk == coord);

        Core::EntityFactory::spawn(tag, kin);
    }
    return true;
}

void ChunkLoader::saveAll() {
    // m_loadedChunks is modified by unloadChunk
    std::vector<ChunkCoord> toSave(m_loadedChunks.begin(), m_loadedChunks.end());
    for (ChunkCoord coord : toSave)
        unloadChunk(coord);
    m_loadedChunks.clear();
}

bool ChunkLoader::chunkIsLoaded(ChunkCoord coord) const {
    return m_loadedChunks.contains(coord);
}
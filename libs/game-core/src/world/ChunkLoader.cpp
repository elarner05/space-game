#include "world/ChunkLoader.h"
#include "components/EntityTypes.h"
#include "core/Core.h"
#include "core/EntityFactory.h"

#include <climits>
#include <cassert>
#include <fstream>
#include <filesystem>

void ChunkLoader::update() {
    ChunkCoord camChunk = Core::camera.kinematics.chunk;
    
    // only recalculate when camera crosses a chunk boundary
    if (camChunk == m_lastCameraChunk) return;
    m_lastCameraChunk = camChunk;

    int ld = GameCamera::loadDistance;

    // collect what should be loaded
    robin_hood::unordered_set<ChunkCoord> desiredChunks;
    for (int dx = -ld; dx <= ld; dx++)
        for (int dy = -ld; dy <= ld; dy++)
            desiredChunks.insert({ camChunk.x + dx, camChunk.y + dy });

    // unload chunks that are no longer in range
    for (auto it = m_loadedChunks.begin(); it != m_loadedChunks.end(); ) {
        if (!desiredChunks.count(*it)) {
            unloadChunk(*it);
            it = m_loadedChunks.erase(it);
        } else {
            ++it;
        }
    }

    // load chunks that aren't loaded yet
    for (const ChunkCoord& coord : desiredChunks) {
        if (!m_loadedChunks.count(coord)) {
            loadChunk(coord);
            m_loadedChunks.insert(coord);
        }
    }
}

// File layout per chunk:
// [uint32] versionNumber
// [uint32] entityCount
// per entity:
//   [uint8]  entityTypeID     (index into a registry of EntityTypes)
//   Kinematics                (plain memcpy)



ChunkLoader::ChunkLoader(const std::string& savePath)
    : m_savePath(savePath)
    , m_lastCameraChunk({INT_MIN, INT_MIN}) // forces first update to load
{
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
void ChunkLoader::unloadChunk(ChunkCoord coord) {
    auto it = Core::chunkMap.find(coord);
    if (it == Core::chunkMap.end() || it->second.empty()) return;

    std::string path = chunkPath(m_savePath, coord);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) return;

    file.write(reinterpret_cast<const char*>(&ChunkLoader::CHUNK_FORMAT_VERSION),
               sizeof(ChunkLoader::CHUNK_FORMAT_VERSION));

    uint32_t count = 0;
    for (EntityID id : it->second) {
        size_t idx = Core::slots[id.index].arrayIndex;
        if ((Core::entityFlagTable[idx] & EntityFlags::Persistent) == EntityFlags::None)
            count++;
    }
    if (count == 0) {
        deleteChunkFile(coord);
        while (true) {
            auto it2 = Core::chunkMap.find(coord);
            if (it2 == Core::chunkMap.end() || it2->second.empty()) break;
            it2->second.erase(it2->second.begin());
        }
        return;
    }

    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (EntityID id : it->second) {
        size_t idx = Core::slots[id.index].arrayIndex;
        if ((Core::entityFlagTable[idx] & EntityFlags::Persistent) != EntityFlags::None) continue;
        EntityTag tag = Core::entityTagTable[idx];
        file.write(reinterpret_cast<const char*>(&tag), sizeof(EntityTag));
        const Kinematics& kin = Core::kinematicsSystem.m_entities[idx];
        file.write(reinterpret_cast<const char*>(&kin), sizeof(Kinematics));
    }

    while (true) {
        auto it2 = Core::chunkMap.find(coord);
        if (it2 == Core::chunkMap.end() || it2->second.empty()) break;
        EntityID front = it2->second.front();
        size_t idx = Core::slots[front.index].arrayIndex;
        if ((Core::entityFlagTable[idx] & EntityFlags::Persistent) != EntityFlags::None) {
            it2->second.erase(it2->second.begin());
            continue;
        }
        Core::unregisterEntity(front);
    }
}

void ChunkLoader::deleteChunkFile(ChunkCoord coord) {
    std::string path = chunkPath(m_savePath, coord);
    if (std::filesystem::exists(path)) {
        std::filesystem::remove(path);
    }
}

void ChunkLoader::deserializeChunk(ChunkCoord coord, std::ifstream& file) {
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (uint32_t i = 0; i < count; i++) {
        EntityTag tag{};
        file.read(reinterpret_cast<char*>(&tag), sizeof(EntityTag));

        Kinematics kin{};
        file.read(reinterpret_cast<char*>(&kin), sizeof(Kinematics));

        Core::EntityFactory::spawn(tag, kin);
    }
}

void ChunkLoader::saveAll() {
    // m_loadedChunks is modified by unloadChunk
    std::vector<ChunkCoord> toSave(m_loadedChunks.begin(), m_loadedChunks.end());
    for (ChunkCoord coord : toSave)
        unloadChunk(coord);
    m_loadedChunks.clear();
}

bool ChunkLoader::chunkIsLoaded(ChunkCoord coord) const {
    return m_loadedChunks.count(coord) > 0;
}
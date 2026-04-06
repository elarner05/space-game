#pragma once
#include "ChunkCoord.h"

#include "robin_hood.h"
#include <string>

class ChunkLoader {
public:
    explicit ChunkLoader(const std::string& savePath);

    void update();
    void saveAll();

    static constexpr uint32_t CHUNK_FORMAT_VERSION = 1;
    
private:
    std::string m_savePath;
    
    ChunkCoord m_lastCameraChunk;
    
    void loadChunk(ChunkCoord coord);
    void unloadChunk(ChunkCoord coord);
    bool chunkIsLoaded(ChunkCoord coord) const;
    void deleteChunkFile(ChunkCoord coord);

    void deserializeChunk(ChunkCoord coord, std::ifstream& file);
    
    robin_hood::unordered_set<ChunkCoord> m_loadedChunks;
};
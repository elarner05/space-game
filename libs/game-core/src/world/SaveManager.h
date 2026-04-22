#pragma once
#include "world/ChunkCoord.h"
#include "components/Kinematics.h"
#include "core/EntityID.h"
#include <string>

// Global save struct; add all necessary save data here and it will save it, just need to load and save as required
struct GlobalSave {
    uint32_t    version;
    ChunkCoord  playerChunk;
    Vector2     playerPosition;
    float       playerRotation;
};

class SaveManager {
public:
    explicit SaveManager(const std::string& savePath);
    
    bool hasSave() const;
    bool load(GlobalSave& out) const;
    bool save(const GlobalSave& data) const;

private:
    std::string m_savePath;
};
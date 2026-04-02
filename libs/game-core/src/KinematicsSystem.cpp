#include "KinematicsSystem.h"
#include "Core.h"

#include <cassert>

KinematicsSystem::KinematicsSystem() = default;

KinematicsSystem::~KinematicsSystem() = default;

void KinematicsSystem::update(float dt) {
    for (size_t i = 0; i < m_entities.size(); i++) {
        Kinematics& kin = m_entities[i];
        ChunkCoord oldChunk = kin.chunk;
        kin.update(dt);

        if (kin.chunk != oldChunk) {
            // remove from old chunk
            auto& oldList = Core::chunkMap[oldChunk];
            oldList.erase(remove(oldList.begin(), oldList.end(), (EntityID)i), oldList.end());

            // insert into new chunk
            Core::chunkMap[kin.chunk].push_back((EntityID)i);
        }
    }
}

Kinematics* KinematicsSystem::registerEntity(Kinematics entity)
{
    m_entities.push_back(Kinematics{std::move(entity)});    
    return &m_entities.back();
}
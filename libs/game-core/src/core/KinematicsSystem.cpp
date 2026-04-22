#include "core/KinematicsSystem.h"
#include "core/Core.h"

#include <cassert>

KinematicsSystem::KinematicsSystem() = default;

KinematicsSystem::~KinematicsSystem() = default;

void KinematicsSystem::update(float dt) {
    for (uint32_t i = 0; i < m_entities.size(); i++) {
        Kinematics& kin = m_entities[i];
        ChunkCoord oldChunk = kin.chunk;
        kin.update(dt);
        Core::getCollider(Core::indexToEntity[i]).setRotation(m_entities[i].rotation);

        if (kin.chunk != oldChunk) {
            EntityID id = Core::indexToEntity[i];
            // remove from old chunk
            auto& oldList = Core::chunkMap[oldChunk];
            oldList.erase(remove(oldList.begin(), oldList.end(), id), oldList.end());

            // insert into new chunk
            Core::chunkMap[kin.chunk].push_back(id);
        }
    }
}

void KinematicsSystem::registerEntity(Kinematics entity)
{
    m_entities.push_back(Kinematics{std::move(entity)});
}
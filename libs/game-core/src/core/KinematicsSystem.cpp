#include "core/KinematicsSystem.h"
#include "core/Core.h"
#include "core/ChunkMapUtil.h"

#include <cassert>

KinematicsSystem::KinematicsSystem() = default;

KinematicsSystem::~KinematicsSystem() = default;

void KinematicsSystem::update(float dt) {

    for (uint32_t i = 0; i < m_entities.size(); i++) {
        Kinematics& kin = m_entities[i];

        kin.update(dt);

        Core::resolveEntityChunk(kin, Core::indexToEntity[i]);
        Core::getCollider(Core::indexToEntity[i]).setRotation(m_entities[i].rotation);

    }
}

void KinematicsSystem::registerEntity(Kinematics entity)
{
    m_entities.push_back(entity);
}
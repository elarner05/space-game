#include "KinematicsSystem.h"

#include <cassert>

KinematicsSystem::KinematicsSystem() = default;

KinematicsSystem::~KinematicsSystem() = default;

void KinematicsSystem::update(float dt)
{
    for (auto& entity : m_entities) {
        entity.update(dt);
    }
}

Kinematics* KinematicsSystem::registerEntity(Kinematics entity)
{
    m_entities.push_back(Kinematics{std::move(entity)});    
    return &m_entities.back();
}

std::deque<Vector2> KinematicsSystem::getPositions() const {
    std::deque<Vector2> positions;
    for (auto entity : m_entities) {
        positions.push_back(entity.position);
    }
    return positions;
}
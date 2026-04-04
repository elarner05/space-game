#include "AnimationSystem.h"
#include <cassert>

AnimationSystem::AnimationSystem() = default;

AnimationSystem::~AnimationSystem() = default;

void AnimationSystem::update(float dt)
{
    for (auto& entity : m_entities) {
        entity.update(dt);
    }
}

EntityID AnimationSystem::registerEntity(Animations entity)
{
    m_entities.push_back(Animations{std::move(entity)});    
    return EntityID{static_cast<uint32_t>(m_entities.size() - 1)};
}

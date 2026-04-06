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

void AnimationSystem::registerEntity(Animations entity)
{
    m_entities.push_back(Animations{std::move(entity)});
}

#include "RenderSystem.h"

#include <cassert>

RenderSystem::RenderSystem() = default;

RenderSystem::~RenderSystem() = default;

void RenderSystem::update(float dt)
{
    // doesn't have a valid update
}

EntityID RenderSystem::registerEntity(Texture2D entity)
{
    m_entities.push_back(entity);    
    return EntityID{static_cast<u_int32_t>(m_entities.size() - 1)};
}

#include "RenderSystem.h"

#include <cassert>

RenderSystem::RenderSystem() = default;

RenderSystem::~RenderSystem() = default;

void RenderSystem::update(float dt)
{
    // doesn't have a valid update
}

Texture2D* RenderSystem::registerEntity(Texture2D entity)
{
    m_entities.push_back(entity);    
    return &m_entities.back();
}

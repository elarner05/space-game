#include "RenderSystem.h"

#include <cassert>

RenderSystem::RenderSystem() = default;

RenderSystem::~RenderSystem() = default;

void RenderSystem::update(float dt)
{
    // doesn't have a valid update
}

void RenderSystem::registerEntity(Texture2D entity)
{
    m_entities.push_back(entity);
}

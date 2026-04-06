#include "ColliderSystem.h"

#include <cassert>
#include <iostream>
#include <cmath>

ColliderSystem::ColliderSystem() = default;

ColliderSystem::~ColliderSystem() = default;

bool ColliderSystem::checkNeedsRotationUpdate() {
    for (size_t i = 0; i < m_entities.size(); ++i) {
        if (m_entities[i].needsRotationUpdate) {
            // std::cout << "Rotation update needed for entity " << i << std::endl;
            return true;
        }
    }
    return false;
}

void ColliderSystem::applyRotations() {
    for (size_t i = 0; i < m_entities.size(); ++i) {
        if (m_entities[i].needsRotationUpdate) {
            // apply new rotation to each collider
            for (int c = 0; c < m_original_entities[i].colliderCount; ++c) {
                Collider& col = m_entities[i].colliders[c];
                Collider& originalCol = m_original_entities[i].colliders[c];

                // rotate offset
                float cosRot = cosf(m_entities[i].rotation);
                float sinRot = sinf(m_entities[i].rotation);
                Vector2 rotatedOffset = {
                    originalCol.offset.x * cosRot - originalCol.offset.y * sinRot,
                    originalCol.offset.x * sinRot + originalCol.offset.y * cosRot
                };
                // Keep offset as float - will be rounded at draw time
                col.offset = rotatedOffset;

                // rotate vertices if not a circle collider
                if (col.count > 0) {
                    for (unsigned char v = 0; v < col.count; ++v) {
                        Vector2 vert = originalCol.verts[v];
                        Vector2 rotatedVert = {
                            vert.x * cosRot - vert.y * sinRot,
                            vert.x * sinRot + vert.y * cosRot
                        };
                        col.verts[v] = {rotatedVert.x, rotatedVert.y};
                    }
                }
            }
            m_entities[i].needsRotationUpdate = false;
        }
    }
}

void ColliderSystem::update(float dt)
{
    if (checkNeedsRotationUpdate()) {
        applyRotations();
    }
}

void ColliderSystem::registerEntity(CompoundCollider entity)
{
    m_original_entities.push_back(CompoundCollider{(entity)}); 
    m_entities.push_back(CompoundCollider{(entity)});
}

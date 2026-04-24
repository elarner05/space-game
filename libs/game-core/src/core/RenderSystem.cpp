#include "core/RenderSystem.h"

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

#include "core/Core.h"
#include "components/Animation.h"
#include "utils/debug_flags.h"


void Core::drawEntities() {

    for (size_t i = 0; i < Core::animationSystem.m_entities.size(); ++i) {
        Animations& anim = Core::animationSystem.m_entities[i];
        Kinematics& kin = Core::kinematicsSystem.m_entities[i];
        CompoundCollider& col = Core::colliderSystem.m_entities[i];
        Texture2D& tex = Core::renderSystem.m_entities[i];

        Vector2 screenPos = kin.localPositionRelativeTo(
            camera.kinematics.chunk,
            camera.kinematics.localPosition
        );

        DrawTexturePro(tex, anim.getSource(),
            Rectangle{ screenPos.x, screenPos.y, anim.dimensions.x, anim.dimensions.y },
            Vector2{ anim.origin.x, anim.origin.y },
            (float)kin.rotation * RAD2DEG, RAYWHITE);


        if (Debug::showHitboxes())
            col.drawDebug(screenPos, RED);
        if (Debug::showEntityOrigins())
            DrawCircle(screenPos.x, screenPos.y, 2, RED);
        if (Core::Debug::showVelocities()) {
            Vector2 start = screenPos;
            Vector2 end   = { start.x + kin.velocity.x, start.y + kin.velocity.y };
            DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, BLUE);
            Vector2 dir  = Vector2Normalize(Vector2Subtract(end, start));
            Vector2 perp = { -dir.y, dir.x };
            constexpr float HEAD_LENGTH = 8.f;
            constexpr float HEAD_WIDTH  = 4.f;
            Vector2 tip   = end;
            Vector2 base  = Vector2Subtract(tip, Vector2Scale(dir, HEAD_LENGTH));
            Vector2 left  = Vector2Add(base, Vector2Scale(perp, HEAD_WIDTH));
            Vector2 right = Vector2Subtract(base, Vector2Scale(perp, HEAD_WIDTH));
            DrawTriangle(tip, right, left, BLUE);
        }
    }

}

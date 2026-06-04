#include "core/RenderSystem.h"

#include "core/Core.h"
#include "components/Animation.h"
#include "utils/debug_flags.h"

void DrawHealthBar(Vector2 position, float width, float height, int16_t health) {
    float ratio = (float)health / (float)100;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    DrawRectangleV(position, {width, height}, RED);

    DrawRectangleV(position, {width * ratio, height}, GREEN);

    DrawRectangleLines(position.x, position.y, width, height, WHITE);
}

void Core::drawEntities() {
    for (size_t i = 0; i < Core::animationTable.size(); ++i) {
        Animations& anim = Core::animationTable[i];
        Kinematics& kin = Core::kinematicsTable[i];
        CompoundCollider& col = Core::colliderTable[i];
        Texture2D& tex = Core::textureTable[i];

        Vector2 screenPos = camera.toScreen(kin);

        DrawTexturePro(tex, anim.getSource(),
            Rectangle{ screenPos.x, screenPos.y, anim.dimensions.x*camera.renderZoom, anim.dimensions.y*camera.renderZoom },
            Vector2{ anim.origin.x*camera.renderZoom, anim.origin.y*camera.renderZoom },
            (float)kin.rotation * RAD2DEG, RAYWHITE);

        DrawHealthBar(Vector2{screenPos.x - anim.dimensions.x/2, screenPos.y - anim.dimensions.y/2}, anim.dimensions.x, 5, Core::statsTable[i].health);

        if (Debug::showHitboxes())
            col.drawDebug(screenPos, RED, camera.renderZoom);
        if (Debug::showEntityOrigins())
            DrawCircle(screenPos.x, screenPos.y, 2, RED);
        if (Core::Debug::showVelocities()) {
            Vector2 start = screenPos;
            Vector2 end = { start.x + kin.velocity.x, start.y + kin.velocity.y };
            DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, BLUE);
            Vector2 dir  = Vector2Normalize(Vector2Subtract(end, start));
            Vector2 perp = { -dir.y, dir.x };
            constexpr float HEAD_LENGTH = 8.f;
            constexpr float HEAD_WIDTH  = 4.f;
            Vector2 tip = end;
            Vector2 base = Vector2Subtract(tip, Vector2Scale(dir, HEAD_LENGTH));
            Vector2 left = Vector2Add(base, Vector2Scale(perp, HEAD_WIDTH));
            Vector2 right = Vector2Subtract(base, Vector2Scale(perp, HEAD_WIDTH));
            DrawTriangle(tip, right, left, BLUE);
        }
    }
}

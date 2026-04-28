#include "input/InputHandler.h"
#include "input/InputKeyConsume.h"
#include "utils/debug_flags.h"
#include "core/EntityID.h"
#include "core/Core.h"
#include "core/ChunkMapUtil.h"

constexpr float THRUST = 50.f;
constexpr float ROT_SPEED = 3.f;

bool Core::Input::handleSpaceshipInput(EntityID id, float dt) {
    Kinematics& kin = Core::getKinematics(id);
    CompoundCollider& col = Core::getCollider(id);
    bool pressed = false;

    Core::Input::Consume::update(); // update consumed keys at start of frame

    if (IsKeyDown(KEY_W)) {
        kin.accelerate(dt, THRUST);
        pressed = true;
    }
    if (IsKeyDown(KEY_A)) {
        kin.changeRotation(-ROT_SPEED * dt);
        kin.setRotationSpeed(0);
        col.setRotation(kin.rotation);
        pressed = true;
    }
    if (IsKeyDown(KEY_D)) {
        kin.changeRotation(+ROT_SPEED * dt);
        kin.setRotationSpeed(0);
        col.setRotation(kin.rotation);
        pressed = true;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        Vector2 screenPos = Core::camera.toScreen(kin);
        kin.accelerateRotation(screenPos, mouse, dt, 10.f);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Core::setEntityChunk(kin, id, ChunkCoord{0, 0});
    }

    if (Core::Input::Consume::pressed(KEY_C)) {
        Core::Debug::showHitboxes() = !Core::Debug::showHitboxes();
    }

    if (Core::Input::Consume::pressed(KEY_DOWN)) {
        Core::camera.renderZoom -=0.2;
        if (Core::camera.renderZoom < 0.2f) Core::camera.renderZoom = 0.2f;
    }
    if (Core::Input::Consume::pressed(KEY_UP)) {
        Core::camera.renderZoom +=0.2;
    }

    return pressed;
}
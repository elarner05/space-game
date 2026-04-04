#include "InputHandler.h"
#include "EntityID.h"
#include "Core.h"

constexpr float THRUST = 50.f;
constexpr float ROT_SPEED = 3.f;

bool Core::Input::handleSpaceshipInput(EntityID id, float dt) {
    Kinematics& kin = Core::getKinematics(id);
    CompoundCollider& col = Core::getCollider(id);
    bool pressed = false;

    if (IsKeyDown(KEY_W)) {
        kin.accelerate(dt, THRUST);
        pressed = true;
    }
    if (IsKeyDown(KEY_A)) {
        kin.changeRotation(-ROT_SPEED * dt);
        col.setRotation(kin.rotation);
        pressed = true;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        Vector2 screenPos = Core::camera.toScreen(kin);
        kin.accelerateRotation(screenPos, mouse, dt, 10.f);
    }

    return pressed;
}
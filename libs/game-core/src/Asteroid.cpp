#include "Asteroid.h"
#include "TextureManager.h"



Asteroid::Asteroid(float x, float y, const char* filepath, const char* metapath, const char* colliderFilepath)
    : texture(*Core::registerComponent(TextureManager::loadTexture(filepath))),
    animations(Core::registerComponent(Animations(metapath))), 
    kinematics(Core::registerComponent(Kinematics{{0, 0}, {x, y}, {1, 1}, 1000, 0, 0, 0})),
    colliders(Core::registerComponent(CompoundCollider(colliderFilepath)))
{
    kinematics->computeAndSetBoundingRadius(*colliders);
}

Asteroid::Asteroid(float x, float y, float velocity_x, float velocity_y, const char* filepath, const char* metapath, const char* colliderFilepath)
    : texture(*Core::registerComponent(TextureManager::loadTexture(filepath))),
    animations(Core::registerComponent(Animations(metapath))), 
    kinematics(Core::registerComponent(Kinematics{{0, 0}, {x, y}, {velocity_x, velocity_y}, 10, 0, 0, 0})),
    colliders(Core::registerComponent(CompoundCollider(colliderFilepath)))
{
    kinematics->computeAndSetBoundingRadius(*colliders);
}


Asteroid::~Asteroid()
{
}

void Asteroid::applyThrust(float dt, float thrust)
{
    kinematics->accelerate(dt, thrust);
}

void Asteroid::draw()
{
    Vector2 screenPos = kinematics->localPositionRelativeTo(
        Core::camera.kinematics.chunk,
        Core::camera.kinematics.localPosition
    );

    DrawTexturePro(texture, animations->getSource(),
        Rectangle{ screenPos.x, screenPos.y, animations->dimensions.x, animations->dimensions.y },
        Vector2{ animations->origin.x, animations->origin.y },
        (float)kinematics->rotation * RAD2DEG, RAYWHITE);

    if (Core::Debug::showHitboxes())
        colliders->drawDebug(screenPos, RED);
    if (Core::Debug::showEntityOrigins())
        DrawCircle((int)screenPos.x, (int)screenPos.y, 2, RED);
    if (Core::Debug::showVelocities()) {
        Vector2 start = screenPos;
        Vector2 end   = { start.x + kinematics->velocity.x, start.y + kinematics->velocity.y };
        DrawLine(start.x, start.y, end.x, end.y, BLUE);
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

void Asteroid::update(float dt)
{
    //applyThrust(dt, Asteroid::sampleThrust)
    // kinematics->update(dt);
    // animations->update(dt);
    colliders->setRotation(kinematics->rotation);
}

void Asteroid::reset()
{

}

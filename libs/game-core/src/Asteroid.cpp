#include "Asteroid.h"
#include "TextureManager.h"



Asteroid::Asteroid(float x, float y, const char* filepath, const char* metapath, const char* colliderFilepath)
    : texture(TextureManager::loadTexture(filepath)),
    animations(Core::registerComponent(Animations(metapath))), 
    kinematics(Core::registerComponent(Kinematics{{x, y}, {1, 1}, 1000, 0, 0, 0})),
    colliders(Core::registerComponent(CompoundCollider(colliderFilepath)))
{
    kinematics->computeAndSetBoundingRadius(*colliders);
}

Asteroid::Asteroid(float x, float y, float velocity_x, float velocity_y, const char* filepath, const char* metapath, const char* colliderFilepath)
    : texture(TextureManager::loadTexture(filepath)),
    animations(Core::registerComponent(Animations(metapath))), 
    kinematics(Core::registerComponent(Kinematics{{x, y}, {velocity_x, velocity_y}, 10, 0, 0, 0})),
    colliders(Core::registerComponent(CompoundCollider(colliderFilepath)))
{
    kinematics->computeAndSetBoundingRadius(*colliders);
}


Asteroid::~Asteroid()
{
}

float Asteroid::getX() const
{
    return kinematics->position.x;
}

float Asteroid::getY() const
{
    return kinematics->position.y;
}

void Asteroid::applyThrust(float dt, float thrust)
{
    kinematics->accelerate(dt, thrust);
}

void Asteroid::draw()
{
    DrawTexturePro(texture, animations->getSource(), Rectangle{ kinematics->position.x, kinematics->position.y, animations->dimensions.x, animations->dimensions.y }, Vector2{ animations->origin.x, animations->origin.y }, (float)kinematics->rotation*RAD2DEG, RAYWHITE);

    if (Core::Debug::showHitboxes())
        colliders->drawDebug(kinematics->position, RED);
    if (Core::Debug::showEntityOrigins())
        DrawCircle(kinematics->position.x, kinematics->position.y, 2, RED);
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

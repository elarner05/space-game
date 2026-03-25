#include "Asteroid.h"
#include "TextureManager.h"



Asteroid::Asteroid(float x, float y, const char* filepath, const char* metapath)
    : animations(metapath)
{
    texture = TextureManager::loadTexture(filepath);
    kinematics = { { x,  y }, {0,0}, 0 };
}

Asteroid::Asteroid(float x, float y, float velocity_x, float velocity_y, const char* filepath, const char* metapath)
    : animations(metapath)
{
    texture = TextureManager::loadTexture(filepath);
    kinematics = { { x,  y }, {velocity_x, velocity_y}, 0 };
}


Asteroid::~Asteroid()
{
}

float Asteroid::getX() const
{
    return kinematics.position.x;
}

float Asteroid::getY() const
{
    return kinematics.position.y;
}

void Asteroid::applyThrust(float dt, float thrust)
{
    kinematics.accelerate(dt, thrust);
}

void Asteroid::draw()
{
    DrawTexturePro(texture, animations.getSource(), Rectangle{ kinematics.position.x, kinematics.position.y, animations.dimensions.x, animations.dimensions.y }, Vector2{ animations.origin.x, animations.origin.y }, (float)kinematics.rotation, RAYWHITE);
}

void Asteroid::update(float dt)
{
    //applyThrust(dt, Asteroid::sampleThrust)
    kinematics.update(dt);
}

void Asteroid::reset()
{

}

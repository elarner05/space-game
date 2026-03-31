#include "Spaceship.h"
#include "TextureManager.h"
#include "Animation.h"
#include "Core.h"
#include <string>

Spaceship::Spaceship(const char* textureFilepath, const char* metaFilepath, const char* colliderFilepath) :
	texture( &TextureManager::loadTexture(textureFilepath) ),
	kinematics ( Core::registerComponent(Kinematics{{100, 100}, {0, 0}, 20, 0, 0, 0}) ),
	pos( &kinematics->position ),
	animations( Core::registerComponent(Animations(metaFilepath)) ),
	colliders( Core::registerComponent(CompoundCollider(colliderFilepath)) )
	{
	kinematics->computeAndSetBoundingRadius(*colliders);
};

Spaceship::~Spaceship() {

}

float Spaceship::getX() const
{
	return kinematics->position.x;
}

float Spaceship::getY() const
{
	return kinematics->position.y;
}

void Spaceship::applyThrust(float dt, float thrust)
{
	kinematics->accelerate(dt, thrust);
}

void Spaceship::changeRotation(float amount)
{
	kinematics->changeRotation(amount);
	colliders->setRotation(kinematics->rotation);
	
}

void Spaceship::accelerateRotation(const Vector2& mouse, const float dt, const float thrust)
{
	kinematics->accelerateRotation({ kinematics->position.x, kinematics->position.y}, mouse, dt, thrust);

}

void Spaceship::draw() {
	DrawTexturePro(*texture, animations->getSource(), Rectangle{kinematics->position.x, kinematics->position.y, animations->dimensions.x, animations->dimensions.y}, Vector2{animations->origin.x, animations->origin.y}, (float)kinematics->rotation*RAD2DEG, RAYWHITE);
	colliders->drawDebug(kinematics->position, RED);
	
}

void Spaceship::update(float dt) {
	colliders->setRotation(kinematics->rotation);
}

void Spaceship::reset() {
	// not implemented
}
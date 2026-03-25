#include "Spaceship.h"
#include "TextureManager.h"
#include "Animation.h"
#include <string>

Spaceship::Spaceship(const char* textureFilepath, const char* metaFilepath, const char* colliderFilepath) :
	texture( TextureManager::loadTexture(textureFilepath) ),
	kinematics ( {{100, 100}, {0, 0}, 0} ),
	pos( &(kinematics.position) ),
	animations( Animations(metaFilepath) ),
	collisions(kinematics.position, colliderFilepath)
	{
};

Spaceship::~Spaceship() {

}

float Spaceship::getX() const
{
	return kinematics.position.x;
}

float Spaceship::getY() const
{
	return kinematics.position.y;
}

void Spaceship::applyThrust(float dt, float thrust)
{
	kinematics.accelerate(dt, thrust);
}

void Spaceship::changeRotation(float amount)
{
	kinematics.changeRotation(amount);
	
}

void Spaceship::accelerateRotation(const Vector2& mouse, const float dt, const float thrust)
{
	kinematics.accelerateRotation({ kinematics.position.x, kinematics.position.y}, mouse, dt, thrust);

}

void Spaceship::draw() {
	DrawTexturePro(texture, animations.getSource(), Rectangle{kinematics.position.x, kinematics.position.y, animations.dimensions.x, animations.dimensions.y}, Vector2{animations.origin.x, animations.origin.y}, (float)kinematics.rotation, RAYWHITE);
	//collisions._drawColliders(RED);
}

void Spaceship::update(float dt) {
	kinematics.update(dt);
	animations.update(dt);
	collisions.setRotation(kinematics.rotation * DEG2RAD);
}

void Spaceship::reset() {
	// not implemented
}
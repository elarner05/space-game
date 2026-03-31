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
	if (Core::Debug::showHitboxes())
        colliders->drawDebug(kinematics->position, RED);
    if (Core::Debug::showEntityOrigins())
        DrawCircle(kinematics->position.x, kinematics->position.y, 2, RED);
	if (Core::Debug::showVelocities()) {
		Vector2 start = kinematics->position;
		Vector2 end   = { start.x + kinematics->velocity.x, start.y + kinematics->velocity.y };

		DrawLine(start.x, start.y, end.x, end.y, BLUE);

		// arrowhead
		Vector2 dir = Vector2Normalize(Vector2Subtract(end, start));
		Vector2 perp = { -dir.y, dir.x }; // perpendicular to direction

		constexpr float HEAD_LENGTH = 8.f;
		constexpr float HEAD_WIDTH  = 4.f;

		Vector2 tip    = end;
		Vector2 base   = Vector2Subtract(tip, Vector2Scale(dir,  HEAD_LENGTH));
		Vector2 left   = Vector2Add     (base, Vector2Scale(perp, HEAD_WIDTH));
		Vector2 right  = Vector2Subtract(base, Vector2Scale(perp, HEAD_WIDTH));

		DrawTriangle(tip, right, left, BLUE);
	}
}

void Spaceship::update(float dt) {
	colliders->setRotation(kinematics->rotation);
}

void Spaceship::reset() {
	// not implemented
}
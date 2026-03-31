#pragma once
#include "raylib.h"
#include "Entity.h"
#include "Animation.h"
#include "Kinematics.h"
#include "CompoundCollider.h"

#include "debug_flags.h"


class Spaceship : public Entity {
public:
	static inline float sampleThrust = 50.f;

	Texture2D* texture;
	Animations* animations;
	Kinematics* kinematics;
	CompoundCollider* colliders;
	Vector2 *pos;
	Spaceship() = delete;
	Spaceship(const char* textureFilepath, const char* metaFilepath, const char* colliderFilepath);
	~Spaceship();

	float getX() const;
	float getY() const;
	void applyThrust(float dt, float thrust);
	void changeRotation(float amount);
	void accelerateRotation(const Vector2& mouse, const float dt, const float thrust);

	void draw() override;
	void update(float dt) override;
	void reset() override;

};
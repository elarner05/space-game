#pragma once
#include "Entity.h"
#include <string>
#include "Animation.h"
#include "Kinematics.h"

#include "Core.h"
#include "debug_flags.h"

class Asteroid : public Entity {
public:
	static inline float sampleThrust = 50.f;
	Texture2D texture;
	Animations* animations;
	Kinematics* kinematics;
	CompoundCollider* colliders;

	// Asteroid() = delete;
	Asteroid(float x, float y, const char* filepath, const char* metapath, const char* colliderFilepath);
	Asteroid(float x, float y, float velocity_x, float velocity_y, const char* filepath, const char* metapath, const char* colliderFilepath);
	~Asteroid();

	float getX() const;
	float getY() const;

	void applyThrust(float dt, float thrust);

	void draw() override;
	void update(float dt) override;
	void reset() override;
		

};
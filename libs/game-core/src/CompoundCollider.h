#pragma once
#include "raylib.h"
#include "gjk.h"

constexpr unsigned char MAX_COLLIDERS = 6;

class CompoundCollider {
public:
	Collider colliders[MAX_COLLIDERS];
	int colliderCount;
	float rotation;
	bool needsRotationUpdate;
	
public:
	CompoundCollider();
	CompoundCollider(const char* filepath);
	CompoundCollider(const CompoundCollider&);	
	CompoundCollider(CompoundCollider&& other) noexcept;	
	CompoundCollider& operator=(const CompoundCollider& other);	
	CompoundCollider& operator=(CompoundCollider&& other) noexcept;	
	~CompoundCollider();

	bool loadColliders(const char* filepath);
	
	
	void drawDebug(const Vector2& pos, const Color c = RED) const;

	void setRotation(float rot);
	
};

bool withinBounds(const Collider& a, const Vector2& pos1, const Collider& b, const Vector2& pos2);
bool updateBounds(Collider& col);
// namespace GJK {
// 	bool collided(const CompoundCollider& left, const Vector2 pos1, const CompoundCollider& right, const Vector2 pos2);
// }
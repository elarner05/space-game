#pragma once
#include "raylib.h"
#include <assert.h>

enum ColliderType
{
    COLLIDER_CIRCLE,
    COLLIDER_AABB,
    COLLIDER_OBB
};

struct CircleCollider
{
    float radius;
};

struct AABBCollider
{
    Vector2 halfExtents;
};

struct OBBCollider
{
    Vector2 halfExtents;
    Vector2 axisX;   // normalized
    Vector2 axisY;   // normalized
};

struct Collider
{
    ColliderType type;
    Vector2 offset;
    Vector2 localOffset; // local-space offset

    union
    {
        CircleCollider circle;
        AABBCollider   aabb;
        OBBCollider    obb;
    };
};

class Collision
{
public:
    static constexpr int MAX_COLLIDERS = 5;

    Collision() = delete;
    Collision(const Vector2& pos);
    Collision(const Vector2& pos, const char* filepath);

    bool loadColliders(const char* filepath);
    bool collided(const Collision& other) const;
    void setRotation(float radians);
    void _drawColliders(Color color);

//private:
    const Vector2& pos;
    Collider colliders[MAX_COLLIDERS];
    int numColliders;
};
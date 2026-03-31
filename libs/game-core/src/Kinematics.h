#pragma once
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include "CompoundCollider.h"

struct Kinematics {
    Vector2 position;
    Vector2 velocity;

    float mass;
    float rotation;
    float angularVelocity;
    float boundingRadius; // for moment of inertia estimation

    void accelerate(float dt, float thrust);
    void update(float dt);
    void changeRotation(float amount);
    void accelerateRotation(const Vector2& origin, const Vector2& mouse, const float dt, const float angularThrust);
    void setRotation(float value);
    void setRotationSpeed(float value);

    float computeEntityBoundingRadius(const CompoundCollider &cc);
    float computeAndSetBoundingRadius(const CompoundCollider &cc);
};
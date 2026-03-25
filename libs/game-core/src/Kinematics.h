#pragma once
#include "raylib.h"
#include <cmath>

struct Kinematics {
    Vector2 position;
    Vector2 velocity;

    float rotation;
    float angularVelocity;

    void accelerate(float dt, float thrust);
    void update(float dt);
    void changeRotation(float amount);
    void accelerateRotation(const Vector2& origin, const Vector2& mouse, const float dt, const float angularThrust);
    void setRotation(float value);
    void setRotationSpeed(float value);
};
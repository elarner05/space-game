#include "Kinematics.h"

void Kinematics::accelerate(float dt, float thrust) {
    float rotationRad = rotation * DEG2RAD;

    Vector2 forward = {
        sinf(rotationRad),
        -cosf(rotationRad)
    };

    Vector2 acceleration = {0};

    acceleration.x = forward.x * thrust;
    acceleration.y = forward.y * thrust;

    velocity.x += acceleration.x * dt;
    velocity.y += acceleration.y * dt;
}

void Kinematics::update(float dt) {
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    
    //angularVelocity *= 0.95f; // dampen angular velocity
    rotation += angularVelocity * dt;
    TraceLog(LOG_INFO, "Kinematics update: pos (%.2f, %.2f), vel (%.2f, %.2f)", position.x, position.y, velocity.x, velocity.y);
}

void Kinematics::changeRotation(float amount)
{
    rotation += amount;
}



void Kinematics::setRotation(float value)
{
    rotation = value;
}

void Kinematics::accelerateRotation(const Vector2 &origin, const Vector2& mouse, const float dt, const float angularThrust)
{
    float deltaX = mouse.x-origin.x;
    float deltaY = mouse.y-origin.y;
    float angleInRadians = atan2(deltaX, -deltaY);

    float targetAngle = angleInRadians * RAD2DEG;

    if (targetAngle < 0) targetAngle += 360.0f;


    float angleDiff = targetAngle - rotation;

    if (angleDiff > 180.0f)  angleDiff -= 360.0f;
    if (angleDiff < -180.0f) angleDiff += 360.0f;

    // accelerate toward target
    angularVelocity += angleDiff * angularThrust * dt;

    // damping (prevents oscillation)
    angularVelocity *= 0.95f;

    // integrate rotation
    rotation += angularVelocity * dt;

    // keep rotation in [0, 360)
    if (rotation >= 360.0f) rotation -= 360.0f;
    if (rotation < 0.0f)    rotation += 360.0f;
}

void Kinematics::setRotationSpeed(float value)
{
    angularVelocity = value;
}

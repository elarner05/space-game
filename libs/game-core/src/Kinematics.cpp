#include "Kinematics.h"

void Kinematics::accelerate(float dt, float thrust) {
    

    Vector2 forward = {
        sinf(rotation),
        -cosf(rotation)
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

    float targetAngle = angleInRadians;

    if (targetAngle < 0) targetAngle += 2*PI;


    float angleDiff = targetAngle - rotation;

    if (angleDiff > PI)  angleDiff -= 2*PI;
    if (angleDiff < -PI) angleDiff += 2*PI;

    // accelerate toward target
    angularVelocity += angleDiff * angularThrust * dt;

    // damping (prevents oscillation)
    angularVelocity *= 0.95f;

    // integrate rotation
    rotation += angularVelocity * dt;

    // keep rotation in [0, 360)
    if (rotation >= 2*PI) rotation -= 2*PI;
    if (rotation < 0.0f)    rotation += 2*PI;
}

void Kinematics::setRotationSpeed(float value)
{
    angularVelocity = value;
}

float Kinematics::computeEntityBoundingRadius(const CompoundCollider& cc)
{
    float maxR = 0.f;
    for (int i = 0; i < cc.colliderCount; i++)
    {
        const Collider& col = cc.colliders[i];
        float offsetDist = Vector2Length(col.offset); // offset from entity origin

        // furthest possible point = offset + collider's own bounding radius
        float r = offsetDist + col.radius;
        if (r > maxR) maxR = r;
    }
    return maxR;
}
float Kinematics::computeAndSetBoundingRadius(const CompoundCollider& cc)
{
    boundingRadius = this->computeEntityBoundingRadius(cc);
    return boundingRadius;
}
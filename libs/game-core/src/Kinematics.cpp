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
    localPosition.x += velocity.x * dt;
    localPosition.y += velocity.y * dt;
    
    //angularVelocity *= 0.95f; // dampen angular velocity
    rotation += angularVelocity * dt;
    rotation = fmodf(rotation, 2*PI);
    if (rotation < 0) rotation += 2*PI;
    resolveChunk();
}

void Kinematics::changeRotation(float amount)
{
    rotation += amount;
    rotation = fmodf(rotation, 2*PI);
    if (rotation < 0) rotation += 2*PI;
}



void Kinematics::setRotation(float value)
{
    rotation = value;
    rotation = fmodf(rotation, 2*PI);
    if (rotation < 0) rotation += 2*PI;
    
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
    rotation = fmodf(rotation, 2*PI);
    if (rotation < 0) rotation += 2*PI;
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

void Kinematics::resolveChunk() {
    // floor-divide to find how many chunks we've crossed
    int deltaX = (int)floorf(localPosition.x / CHUNK_SIZEF);
    int deltaY = (int)floorf(localPosition.y / CHUNK_SIZEF);

    chunk.x += deltaX;
    chunk.y += deltaY;

    // keep localPosition within [0, CHUNK_SIZE)
    localPosition.x -= deltaX * CHUNK_SIZEF;
    localPosition.y -= deltaY * CHUNK_SIZEF;
}

Vector2 Kinematics::localPositionRelativeTo(ChunkCoord c, Vector2 position) const {
    float rx = (float)(chunk.x - c.x) * CHUNK_SIZEF + localPosition.x - position.x;
    float ry = (float)(chunk.y - c.y) * CHUNK_SIZEF + localPosition.y - position.y;
    return { rx, ry };
}

Vector2 Kinematics::localPositionRelativeTo(const Kinematics& other) const {
    float rx = (float)(chunk.x - other.chunk.x) * CHUNK_SIZEF + localPosition.x;
    float ry = (float)(chunk.y - other.chunk.y) * CHUNK_SIZEF + localPosition.y;
    return { rx, ry };
}
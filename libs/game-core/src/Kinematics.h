#pragma once
#include "raylib.h"
#include "raymath.h"

#include "ChunkCoord.h"
#include "CompoundCollider.h"

#include "robin_hood.h"

#include <cmath>
#include <algorithm>

constexpr int   CHUNK_SIZE = 1024; // world units per chunk side
constexpr float CHUNK_SIZEF = static_cast<float>(CHUNK_SIZE);




struct Kinematics {
    ChunkCoord chunk;
    Vector2 localPosition;

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

    bool resolveChunk();

    Vector2 localPositionRelativeTo(ChunkCoord c, Vector2 position) const;
    Vector2 localPositionRelativeTo(const Kinematics& other) const;
};
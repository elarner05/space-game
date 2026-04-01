#pragma once
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include "CompoundCollider.h"

constexpr int   CHUNK_SIZE = 1024; // world units per chunk side
constexpr float CHUNK_SIZEF = static_cast<float>(CHUNK_SIZE);
struct ChunkCoord{
    int x;
    int y;

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }

    // Manhattan-style proximity check for broad-phase cull
    bool isAdjacentTo(const ChunkCoord& other) const {
        return abs(x - other.x) <= 1 && abs(y - other.y) <= 1;
    }

    // Useful for chunk loading/unloading systems later
    int chebyshevDistance(const ChunkCoord& other) const {
        return std::max(abs(x - other.x), abs(y - other.y));
    }
};

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

    void resolveChunk();

    Vector2 localPositionRelativeTo(ChunkCoord c, Vector2 position) const;
    Vector2 localPositionRelativeTo(const Kinematics& other) const;
};
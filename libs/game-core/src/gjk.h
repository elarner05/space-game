#pragma once
#include "raylib.h"

constexpr unsigned char MAX_VERTICES = 4;

struct Collider
{
    Vector2 verts[MAX_VERTICES]; // vertices of the shape, in local terms
    unsigned count; // number of vertices
    float   radius; // radius of minimum circle containing the shape
    Vector2 offset; // local position of the shape
};

bool gjk(const Collider& first, const Vector2 &posA, const Collider& second, const Vector2 &posB);
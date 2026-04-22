#pragma once
#include "raylib.h"

constexpr unsigned char MAX_VERTICES = 4;

// a single collider, which defines a convex shape for use in GJK/EPA
// count >  0: polygon of `count` vertices
// count == 0: circle with radius `radius`
struct Collider
{
    Vector2 verts[MAX_VERTICES]; // vertices of the shape, in local terms
    unsigned count; // number of vertices
    float   radius; // radius of minimum circle containing the shape
    Vector2 offset; // local position of the shape
};

// the Gilbert-Johnson-Keerthi (GJK) algorithm for collsiion detection between two convex shapes
// returns whether the shapes intersect, and the simplex of the last iteration which encloses the Minkowski origin if they do
bool gjk(const Collider& first, const Vector2 &posA, const Collider& second, const Vector2 &posB, Vector2 outSimplex[3]);

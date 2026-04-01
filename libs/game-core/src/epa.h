#pragma once
#include "raylib.h"
#include "gjk.h"
#include "Kinematics.h"

// result of the EPA algorithm
struct ContactManifold {
    Vector2 normal;   // unit vector pointing from B into A
    float depth;    // penetration depth in px
    Vector2 contact;  // worldspace contact point (midpoint on the penetrating edge)
    bool valid;    // whether EPA succeeded
};

// the expanding polytope algorithm (EPA) for finding the penetration normal and depth for two intersecting convex shapes, using the result from the gjk algorithm
// (simplex is the three points from the last GJK iteration, that encloses the origin) 
ContactManifold epa( const Collider& a, const Vector2& posA, const Collider& b, const Vector2& posB, Vector2 simplex[3]);

// applies an impulse based collision response to a collsion, based on the epa manifold
// (restitution: 0 = perfectly inelastic, 1 = perfectly elastic)
void resolveCollision(const ContactManifold& manifold, Kinematics* kA, const Vector2& posA, Kinematics* kB, const Vector2& posB, float restitution=0.3f);

// applies a small positional correction to move entities out of penetration, using the epa manifold
void positionalCorrection(const ContactManifold& manifold, Kinematics* kA, Kinematics* kB);
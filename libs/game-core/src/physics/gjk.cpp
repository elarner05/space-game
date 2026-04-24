
#include "physics/gjk.h"
#include "raylib.h"
#include "raymath.h"

#include "utils/profiler.hpp"

// Gilbert-Johnson-Keerthi (GJK) collision detection algorithm in 2D
// http://www.dyn4j.org/2010/04/gjk-gilbert-johnson-keerthi/



static inline Vector2 Vector2Perpendicular(Vector2 v) {
    return Vector2 {v.y, -v.x};
}

// Triple product expansion is used to calculate Vector2Perpendicular normal vectors
// which prefers pointing towards the Origin of the Minkowski space
static inline Vector2 TripProduct(Vector2 a, Vector2 b, Vector2 c) {

    Vector2 r = {0, 0};

    float ac = a.x * c.x + a.y * c.y; // perform a.dot(c)
    float bc = b.x * c.x + b.y * c.y; // perform b.dot(c)

    // perform b * a.dot(c) - a * b.dot(c)
    r.x = b.x * ac - a.x * bc;
    r.y = b.y * ac - a.y * bc;
    return r;
}

// This computes the average center (roughly).
static inline Vector2 AvgPoint(const Collider& col, const Vector2& pos) {
    if (col.count == 0)
        return Vector2Add(pos, col.offset);
    Vector2 avg = {0.f, 0.f};
    for (unsigned i = 0; i < col.count; i++) {
        avg = Vector2Add(avg, col.verts[i]);
    }
    avg = Vector2Scale(avg, 1.0f / col.count);
    return Vector2Add(avg, Vector2Add(pos, col.offset));
}

// Get furthest vertex along a certain direction
static inline size_t FurthestIndex(const Collider& col, Vector2 direction) {

    float       maxProduct = Vector2DotProduct(direction, col.verts[0]);
    size_t      index      = 0;
    for (size_t i          = 1; i < col.count; i++) {
        float product = Vector2DotProduct(direction, col.verts[i]);
        if (product > maxProduct) {
            maxProduct = product;
            index      = i;
        }
    }
    return index;
}

// Minkowski sum support function for GJK
// Gets point on collider for the current direction
static inline Vector2 SupportPoint(const Collider& c, const Vector2& pos, Vector2 dir)
{
    Vector2 base = Vector2Add(pos, c.offset);

    if (c.count > 0) // polygons
    {
        size_t i = FurthestIndex(c, dir);
        return Vector2Add(base, c.verts[i]);
    }
    else // circles
    {
        Vector2 n = Vector2Normalize(dir);
        return Vector2Add(base, Vector2Scale(n, c.radius));
    }
}

// gets Minkowski sum
static inline Vector2 Support(const Collider& a, const Vector2& posA, const Collider& b, const Vector2& posB, Vector2 dir)
{
    return Vector2Subtract(
        SupportPoint(a, posA, dir),
        SupportPoint(b, posB, Vector2Negate(dir))
    );
}




static constexpr int GJK_MAX_ITERS = 20;

// The GJK yes/no test
bool gjk(const Collider& first, const Vector2& posA, const Collider& second, const Vector2& posB, Vector2 outSimplex[3])
{
    ZoneScoped;

    size_t  index = 0; // index of current vertex of simplex
    Vector2 a, b, c, direction, ao, ab, ac, abperp, acperp = {0};

    Vector2 position1 = AvgPoint(first, posA); // not a CoG but
    Vector2 position2 = AvgPoint(second, posB);// it's ok for GJK
     

    // initial direction from the center of 1st body to the center of 2nd body
    direction = Vector2Subtract(position1, position2);

    // if initial direction is zero, set it to any arbitrary axis (we choose X)
    if ((direction.x == 0) && (direction.y == 0)) {
        direction.x = 1.f;
    }

    // set the first support as initial point of the new simplex
    a = outSimplex[0] = Support(first, posA, second, posB, direction);

    if (Vector2DotProduct(a, direction) <= 0) {
        return false;
    } // no collision

    direction = Vector2Negate(a); // The next search direction is always towards the origin, so the next search direction is negate(a)

    int iter_count = 0;

    while (iter_count++ < GJK_MAX_ITERS) {
        a = outSimplex[++index] = Support(
                first, posA, second, posB, direction);

        if (Vector2DotProduct(a, direction) <= 1e-6f)
            return false; // no collision

        ao = Vector2Negate(a); // from point A to Origin is just negative A

        // simplex has 2 points (a line segment, not a triangle yet)
        if (index < 2) {
            b             = outSimplex[0];
            ab            = Vector2Subtract(b, a); // from point A to B
            direction     = TripProduct(
                    ab, ao, ab); // normal to AB towards Origin
            if (Vector2LengthSqr(direction) == 0)
                direction = Vector2Perpendicular(ab);
            continue; // skip to next iteration
        }

        b  = outSimplex[1];
        c  = outSimplex[0];
        ab = Vector2Subtract(b, a); // from point A to B
        ac = Vector2Subtract(c, a); // from point A to C

        acperp = TripProduct(ab, ac, ac);

        if (Vector2DotProduct(acperp, ao) >= 0) {

            direction = acperp; // new direction is normal to AC towards Origin

        }
        else {

            abperp = TripProduct(ac, ab, ab);

            if (Vector2DotProduct(abperp, ao) < 0)
                return true; // collision

            outSimplex[0] = outSimplex[1]; // swap first element (point C)

            direction = abperp; // new direction is normal to AB towards Origin
        }

        outSimplex[1] = outSimplex[2]; // swap element in the middle (point B)
        --index;
    }

    return false; // not found within GJK_MAX_ITERS iterations
}


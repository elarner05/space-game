#include "epa.h"
#include "raymath.h"
#include <cfloat>
#include <vector>

// internal helpers

// 2D "cross product"; the scalar z-component of (a x b)
static inline float Cross2(Vector2 a, Vector2 b) {
    return a.x * b.y - a.y * b.x;
}

// Get furthest vertex along a certain direction
static inline size_t FurthestIndex(const Collider& col, Vector2 direction) {

    float maxProduct = Vector2DotProduct(direction, col.verts[0]);
    size_t index = 0;
    for (size_t i = 1; i < col.count; i++) {
        float product = Vector2DotProduct(direction, col.verts[i]);
        if (product > maxProduct) {
            maxProduct = product;
            index = i;
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





// EPA expands the GJK simplex (2-dimensional triangle) by repeatedly finding the
// closest edge to the origin and adding a new support point in that direction.
// When the new point is no closer to the origin than the edge itself, we have
// found the penetration normal and depth.

static constexpr int EPA_MAX_ITERS = 32;
static constexpr float EPA_TOLERANCE = 1e-4f;

ContactManifold epa(const Collider& a, const Vector2& posA, const Collider& b, const Vector2& posB, Vector2 simplex[3])
{
    ContactManifold result{};
    result.valid = false;

    // mutable polygon from the GJK simplex, expanded by epa
    std::vector<Vector2> poly = { simplex[0], simplex[1], simplex[2] };

    // EPA requires CCW winding so that outward normals point away from the origin
    // The signed area of the triangle is positive if CCW, negative if CW
    float signedArea = Cross2(Vector2Subtract(poly[1], poly[0]), Vector2Subtract(poly[2], poly[0]));
    if (signedArea < 0.f) {
        std::swap(poly[0], poly[1]);
    }

    for (int _ = 0; _ < EPA_MAX_ITERS; _++) {

        // first find the closest edge to the origin

        float minDist = FLT_MAX;
        int minIndex = 0;
        Vector2 minNormal{};

        for (int i = 0; i < (int)poly.size(); i++) {
            int j = (i + 1) % (int)poly.size();
            Vector2 a_ = poly[i];
            Vector2 b_ = poly[j];

            Vector2 edge = Vector2Subtract(b_, a_);
            // outward-facing normal
            Vector2 normal = Vector2Normalize(Vector2{ edge.y, -edge.x });

            float dist = Vector2DotProduct(normal, a_);

            // if dist is negative the normal points inward; flip it
            if (dist < 0.f) {
                dist = -dist;
                normal = Vector2Negate(normal);
            }

            if (dist < minDist) {
                minDist = dist;
                minIndex = i;
                minNormal = normal;
            }
        }

        // get the support point along the normal

        Vector2 sup = Support(a, posA, b, posB, minNormal);
        float d = Vector2DotProduct(minNormal, sup);

        // converged
        if (fabsf(d - minDist) < EPA_TOLERANCE) {
            // contact - midpoint of the world-space witness points on each shape

            Vector2 witnessA = SupportPoint(a, posA,  minNormal);
            Vector2 witnessB = SupportPoint(b, posB, Vector2Negate(minNormal));

            result.normal = Vector2Negate(minNormal); // EPA produces normal from A into B, flip to B into A
            result.depth = minDist;
            result.contact = Vector2Scale(Vector2Add(witnessA, witnessB), 0.5f);
            result.valid = true;
            return result;
        }

        // not converged; insert point into poly and continue
        poly.insert(poly.begin() + minIndex + 1, sup);
    }

    // reached iteration limit
    // find the closest edge one more time for the fallback result

    float   minDist  = FLT_MAX;
    Vector2 minNormal{};
    for (int i = 0; i < (int)poly.size(); i++) {
        int j = (i + 1) % (int)poly.size();
        Vector2 edge = Vector2Subtract(poly[j], poly[i]);
        Vector2 normal = Vector2Normalize(Vector2{ edge.y, -edge.x });
        float dist = Vector2DotProduct(normal, poly[i]);
        if (dist < 0.f) { dist = -dist; normal = Vector2Negate(normal); }
        if (dist < minDist) { minDist = dist; minNormal = normal; }
    }

    Vector2 witnessA = SupportPoint(a, posA,  minNormal);
    Vector2 witnessB = SupportPoint(b, posB, Vector2Negate(minNormal));

    result.normal = Vector2Negate(minNormal); // EPA produces normal from A into B, flip to B into A
    result.depth = minDist;
    result.contact = Vector2Scale(Vector2Add(witnessA, witnessB), 0.5f);
    result.valid = true;
    return result;
}




// Collision response

// I = mass * radius^2 * 0.5
static float momentOfInertia(const Kinematics* k, float boundingRadius) {
    return 0.5f * k->mass * boundingRadius * boundingRadius; // disc formula
}

void resolveCollision(const ContactManifold& manifold, Kinematics* kA, const Vector2& posA, Kinematics* kB, const Vector2& posB, float restitution)
{
    if (!manifold.valid) return;

    const Vector2& n = manifold.normal; // points from B into A

    // Moment of inertia (defaults to 1 if mass is zero)
    float iA = (kA->mass > 0.f) ? momentOfInertia(kA, kA->boundingRadius) : 1.f;
    float iB = (kB->mass > 0.f) ? momentOfInertia(kB, kB->boundingRadius) : 1.f;

    float invMA = (kA->mass > 0.f) ? 1.f / kA->mass : 0.f; // 0 = immovable
    float invMB = (kB->mass > 0.f) ? 1.f / kB->mass : 0.f;
    float invIA = (iA > 0.f) ? 1.f / iA : 0.f;
    float invIB = (iB > 0.f) ? 1.f / iB : 0.f;

    // Lever arms (contact point relative to each centre of mass)
    Vector2 rA = Vector2Subtract(manifold.contact, posA);
    Vector2 rB = Vector2Subtract(manifold.contact, posB);

    // Velocity of each body AT the contact point 
    // v_contact = v_linear + omega x r  (in 2D: omega x r = { -omega*r.y, omega*r.x })
    Vector2 vA_contact = { kA->velocity.x - kA->angularVelocity * rA.y, kA->velocity.y + kA->angularVelocity * rA.x };
    Vector2 vB_contact = { kB->velocity.x - kB->angularVelocity * rB.y, kB->velocity.y + kB->angularVelocity * rB.x };

    Vector2 relVel = Vector2Subtract(vA_contact, vB_contact);
    float vn = Vector2DotProduct(relVel, n);

    if (vn > 0.f) return; // if seperating, exit


    float rA_cross_n = Cross2(rA, n);
    float rB_cross_n = Cross2(rB, n);

    // denom accounts for both linear and rotational inertia
    float denom = invMA + invMB
                + (rA_cross_n * rA_cross_n) * invIA
                + (rB_cross_n * rB_cross_n) * invIB;

    if (denom < 1e-10f) return; // degenerate

    float j = -(1.f + restitution) * vn / denom; // impulse scalar

    // linear impulse
    kA->velocity.x += invMA * j * n.x;
    kA->velocity.y += invMA * j * n.y;
    kB->velocity.x -= invMB * j * n.x;
    kB->velocity.y -= invMB * j * n.y;

    // angular impulse
    kA->angularVelocity += (invIA * Cross2(rA, Vector2Scale(n, j)));
    kB->angularVelocity -= (invIB * Cross2(rB, Vector2Scale(n, j)));

    vA_contact = { kA->velocity.x - kA->angularVelocity * rA.y, kA->velocity.y + kA->angularVelocity * rA.x };
    vB_contact = { kB->velocity.x - kB->angularVelocity * rB.y, kB->velocity.y + kB->angularVelocity * rB.x };
    relVel = Vector2Subtract(vA_contact, vB_contact);

    // tangent = relative velocity minus its normal component
    Vector2 tangent = Vector2Subtract(relVel, Vector2Scale(n, Vector2DotProduct(relVel, n)));
    float tLen = Vector2Length(tangent);

    // friction impulse
    if (tLen > 1e-6f) {
        tangent = Vector2Scale(tangent, 1.f / tLen);

        float rA_cross_t = Cross2(rA, tangent);
        float rB_cross_t = Cross2(rB, tangent);

        float denomT = invMA + invMB
                     + (rA_cross_t * rA_cross_t) * invIA
                     + (rB_cross_t * rB_cross_t) * invIB;

        if (denomT > 1e-10f) {
            float jt = -Vector2DotProduct(relVel, tangent) / denomT;

            // coulomb clamp
            constexpr float MU_STATIC = 0.5f;
            constexpr float MU_DYNAMIC = 0.3f;
            Vector2 frictionImpulse;
            if (fabsf(jt) <= j * MU_STATIC) {
                frictionImpulse = Vector2Scale(tangent, jt);
            } else {
                frictionImpulse = Vector2Scale(tangent, -j * MU_DYNAMIC);
            }

            kA->velocity.x += invMA * frictionImpulse.x;
            kA->velocity.y += invMA * frictionImpulse.y;
            kB->velocity.x -= invMB * frictionImpulse.x;
            kB->velocity.y -= invMB * frictionImpulse.y;

            kA->angularVelocity += (invIA * Cross2(rA, frictionImpulse));
            kB->angularVelocity -= (invIB * Cross2(rB, frictionImpulse));
        }
    }
}

// positional correction (baumgarte)
void positionalCorrection(const ContactManifold& manifold, Kinematics* kA, Kinematics* kB)
{
    if (!manifold.valid) return;

    constexpr float SLOP = 1.5f; // pixels of allowed penetration before correction
    constexpr float BIAS = 0.4f; // how aggressively to correct (ideally 0.2 - 0.8)

    float invMA = (kA->mass > 0.f) ? 1.f / kA->mass : 0.f;
    float invMB = (kB->mass > 0.f) ? 1.f / kB->mass : 0.f;
    float total = invMA + invMB;
    if (total < 1e-10f) return;

    float mag = fmaxf(manifold.depth - SLOP, 0.f) / total * BIAS;
    Vector2 correction = Vector2Scale(manifold.normal, mag);

    kA->localPosition = Vector2Add(kA->localPosition, Vector2Scale(correction, invMA));
    kA->resolveChunk();
    kB->localPosition = Vector2Subtract(kB->localPosition, Vector2Scale(correction, invMB));
    kB->resolveChunk();
}
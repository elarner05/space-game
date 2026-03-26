#include "Collision.h"
#include "raymath.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

// ------------------------------------------------------------
// Helper math
// ------------------------------------------------------------

static bool CircleCircle(const Vector2& aPos, const CircleCollider& a,
    const Vector2& bPos, const CircleCollider& b)
{
    float r = a.radius + b.radius;
    return Vector2DistanceSqr(aPos, bPos) <= r * r;
}

static bool AABBAABB(const Vector2& aPos, const AABBCollider& a,
    const Vector2& bPos, const AABBCollider& b)
{
    return
        fabsf(aPos.x - bPos.x) <= (a.halfExtents.x + b.halfExtents.x) &&
        fabsf(aPos.y - bPos.y) <= (a.halfExtents.y + b.halfExtents.y);
}

static bool CircleAABB(const Vector2& cPos, const CircleCollider& c,
    const Vector2& bPos, const AABBCollider& b)
{
    Vector2 d = Vector2Subtract(cPos, bPos);

    float x = Clamp(d.x, -b.halfExtents.x, b.halfExtents.x);
    float y = Clamp(d.y, -b.halfExtents.y, b.halfExtents.y);

    Vector2 closest = { bPos.x + x, bPos.y + y };
    return Vector2DistanceSqr(cPos, closest) <= c.radius * c.radius;
}

// ------------------------------------------------------------
// OBB helpers (SAT)
// ------------------------------------------------------------

static void ProjectOBB(const Vector2& center, const OBBCollider& obb,
    const Vector2& axis, float& min, float& max)
{
    float c = Vector2DotProduct(center, axis);
    float r =
        obb.halfExtents.x * fabsf(Vector2DotProduct(obb.axisX, axis)) +
        obb.halfExtents.y * fabsf(Vector2DotProduct(obb.axisY, axis));

    min = c - r;
    max = c + r;
}

static bool OverlapOnAxis(const Vector2& aPos, const OBBCollider& a,
    const Vector2& bPos, const OBBCollider& b,
    const Vector2& axis)
{
    float minA, maxA, minB, maxB;
    ProjectOBB(aPos, a, axis, minA, maxA);
    ProjectOBB(bPos, b, axis, minB, maxB);
    return !(maxA < minB || maxB < minA);
}

static bool OBBOBB(const Vector2& aPos, const OBBCollider& a,
    const Vector2& bPos, const OBBCollider& b)
{
    return
        OverlapOnAxis(aPos, a, bPos, b, a.axisX) &&
        OverlapOnAxis(aPos, a, bPos, b, a.axisY) &&
        OverlapOnAxis(aPos, a, bPos, b, b.axisX) &&
        OverlapOnAxis(aPos, a, bPos, b, b.axisY);
}

void SetOBBRotation(OBBCollider& obb, float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);

    obb.axisX = { c, s };
    obb.axisY = { -s, c };
}

static void DrawCollider(const Collider& c, const Vector2& worldPos, Color color)
{
    Vector2 center = Vector2Add(worldPos, c.offset);

    switch (c.type)
    {
    case COLLIDER_CIRCLE:
        DrawCircleLines((int)center.x, (int)center.y, c.circle.radius, color);
        break;

    case COLLIDER_AABB:
    {
        Rectangle r = { 0 };
        r.x = center.x - c.aabb.halfExtents.x;
        r.y = center.y - c.aabb.halfExtents.y;
        r.width = c.aabb.halfExtents.x * 2.0f;
        r.height = c.aabb.halfExtents.y * 2.0f;
        DrawRectangleLinesEx(r, 1.0f, color);
        break;
    }

    case COLLIDER_OBB:
    {
        // compute corners from axes
        Vector2 hx = { c.obb.axisX.x * c.obb.halfExtents.x,
                       c.obb.axisX.y * c.obb.halfExtents.x };
        Vector2 hy = { c.obb.axisY.x * c.obb.halfExtents.y,
                       c.obb.axisY.y * c.obb.halfExtents.y };

        Vector2 p0 = { center.x - hx.x - hy.x, center.y - hx.y - hy.y };
        Vector2 p1 = { center.x + hx.x - hy.x, center.y + hx.y - hy.y };
        Vector2 p2 = { center.x + hx.x + hy.x, center.y + hx.y + hy.y };
        Vector2 p3 = { center.x - hx.x + hy.x, center.y - hx.y + hy.y };

        DrawLineV(p0, p1, color);
        DrawLineV(p1, p2, color);
        DrawLineV(p2, p3, color);
        DrawLineV(p3, p0, color);

        // optional: draw axes
        DrawLineV(center,
            { center.x + c.obb.axisX.x * 20.0f,
              center.y + c.obb.axisX.y * 20.0f }, DARKPURPLE);
        DrawLineV(center,
            { center.x + c.obb.axisY.x * 20.0f,
              center.y + c.obb.axisY.y * 20.0f }, GREEN);
        break;
    }
    }
}

Collision::Collision(const Vector2& pos)
    : colliders{}, numColliders(0), pos(pos)
{

}
Collision::Collision(const Vector2& pos, const char* filepath)
    : colliders{}, numColliders(0), pos(pos)
{
    
    bool loaded = loadColliders(filepath);
    assert(loaded);
}

bool Collision::loadColliders(const char* filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        return false;

    std::string line;
    int current = -1;
    numColliders = 0;

    while (std::getline(file, line))
    {
        std::cout << line << std::endl;
        // Trim leading whitespace
        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos)
            continue;

        // Skip comments
        if (line[first] == '#')
            continue;

        // Collider count
        if (line.substr(first, 9) == "colliders")
        {
            std::stringstream ss(line.substr(first));
            std::string key, eq;
            ss >> key >> eq >> numColliders;
            std::cout << numColliders << std::endl;

            if (numColliders <= 0 || numColliders > MAX_COLLIDERS)
                return false;

            continue;
        }

        // Collider block header
        if (line[first] == '[')
        {
            if (sscanf(line.c_str(), "[collider%d]", &current) != 1)
                return false;

            if (current < 0 || current >= numColliders)
                return false;

            colliders[current] = {};
            continue;
        }

        if (current < 0)
            continue;

        Collider& c = colliders[current];

        // Key-value parsing
        std::stringstream ss(line.substr(first));
        std::string key;
        ss >> key;

        if (key == "type")
        {
            std::string value;
            ss >> value >> value; // skip '='
            if (value == "circle")      c.type = COLLIDER_CIRCLE;
            else if (value == "aabb")   c.type = COLLIDER_AABB;
            else if (value == "obb")    c.type = COLLIDER_OBB;
            else return false;
        }
        else if (key == "offset")
        {
            ss >> key >> c.localOffset.x >> c.localOffset.y;
            c.offset = c.localOffset;
        }
        else if (key == "radius")
        {
            ss >> key >> c.circle.radius;
        }
        else if (key == "halfExtents")
        {
            ss >> key >> c.aabb.halfExtents.x >> c.aabb.halfExtents.y;
        }
        else if (key == "axisX")
        {
            ss >> key >> c.obb.axisX.x >> c.obb.axisX.y;
        }
        else if (key == "axisY")
        {
            ss >> key >> c.obb.axisY.x >> c.obb.axisY.y;
        }
    }

    file.close();

    // Final validation & normalization
    for (int i = 0; i < numColliders; ++i)
    {
        Collider& c = colliders[i];

        if (c.type == COLLIDER_OBB)
        {
            c.obb.axisX = Vector2Normalize(c.obb.axisX);
            c.obb.axisY = Vector2Normalize(c.obb.axisY);

            float dot = Vector2DotProduct(c.obb.axisX, c.obb.axisY);
            if (fabsf(dot) > 0.01f)
                return false;
        }
    }

    return true;
}


bool Collision::collided(const Collision& other) const
{
    for (int i = 0; i < numColliders; ++i)
    {
        for (int j = 0; j < other.numColliders; ++j)
        {
            const Collider& a = colliders[i];
            const Collider& b = other.colliders[j];

            Vector2 aPos = Vector2Add(this->pos, a.localOffset);
            Vector2 bPos = Vector2Add(other.pos, b.localOffset);

            if (a.type == COLLIDER_CIRCLE && b.type == COLLIDER_CIRCLE)
            {
                if (CircleCircle(aPos, a.circle, bPos, b.circle))
                    return true;
            }
            else if (a.type == COLLIDER_AABB && b.type == COLLIDER_AABB)
            {
                if (AABBAABB(aPos, a.aabb, bPos, b.aabb))
                    return true;
            }
            else if (a.type == COLLIDER_CIRCLE && b.type == COLLIDER_AABB)
            {
                if (CircleAABB(aPos, a.circle, bPos, b.aabb))
                    return true;
            }
            else if (a.type == COLLIDER_AABB && b.type == COLLIDER_CIRCLE)
            {
                if (CircleAABB(bPos, b.circle, aPos, a.aabb))
                    return true;
            }
            else if (a.type == COLLIDER_OBB && b.type == COLLIDER_OBB)
            {
                if (OBBOBB(aPos, a.obb, bPos, b.obb))
                    return true;
            }
        }
    }
    return false;
}

Vector2 Rotate(Vector2 v, float c, float s)
{
    return { v.x * c - v.y * s, v.x * s + v.y * c };
}
void Collision::setRotation(float radians)
{
    float c = cosf(radians);
    float s = sinf(radians);

    for (int i = 0; i < numColliders; ++i)
    {
        Collider& col = colliders[i];

        // Rotate from ORIGINAL local offset every time
        col.offset = {
            col.localOffset.x * c - col.localOffset.y * s,
            col.localOffset.x * s + col.localOffset.y * c
        };

        if (col.type == COLLIDER_OBB)
        {
            col.obb.axisX = { c, s };
            col.obb.axisY = { -s, c };
        }
        printf("local=(%.2f, %.2f) offset=(%.2f, %.2f)\n",
            col.localOffset.x, col.localOffset.y,
            col.offset.x, col.offset.y);
    }
}

void Collision::_drawColliders(Color color)
{
    for (int i = 0; i < numColliders; ++i)
    {
        DrawCollider(colliders[i], pos, color);
    }
}
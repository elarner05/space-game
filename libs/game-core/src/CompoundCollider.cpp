#include "CompoundCollider.h"
#include "raymath.h"
#include "raylib.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

bool withinBounds(const Collider& a, const Vector2& pos1, const Collider& b, const Vector2& pos2)
{
    return a.radius + b.radius > Vector2Distance(Vector2Add(pos1, a.offset), Vector2Add(pos2, b.offset));
}

bool updateBounds(Collider& col) 
{
    if (col.count == 0 && col.radius == 0.f) 
    {
        return false; // invalid collider?
    }

    for (unsigned char i{ 0 }; i < col.count; i++) 
    {
        float disToVert = Vector2Distance(col.verts[i], col.offset);
        if (disToVert > col.radius) 
        {
            col.radius = disToVert;
        }
    }
    if (col.radius == 0)
        return false;
    return true;

}

CompoundCollider::CompoundCollider()
    :colliders{nullptr}, colliderCount(0), rotation(0.f), needsRotationUpdate(false)
{
    colliders = new Collider[MAX_COLLIDERS]{ 0 };
}

CompoundCollider::CompoundCollider(const char* filepath)
    :colliders{ nullptr }, colliderCount(0), rotation(0.f), needsRotationUpdate(false)
{
    colliders = new Collider[MAX_COLLIDERS]{ 0 };
    loadColliders(filepath);
}
CompoundCollider::CompoundCollider(const CompoundCollider& other)
    :colliders{ nullptr }, colliderCount(0), rotation(0.f), needsRotationUpdate(false)
{
    colliders = new Collider[MAX_COLLIDERS]{ 0 };
    colliderCount = other.colliderCount;
    for (int i = 0; i < other.colliderCount; ++i)
    {
        // copy collider
        memcpy(&colliders[i], &other.colliders[i], sizeof(Collider));
    }
}

CompoundCollider::CompoundCollider(CompoundCollider&& other) noexcept
    : colliders(other.colliders), colliderCount(other.colliderCount), rotation(other.rotation), needsRotationUpdate(other.needsRotationUpdate)
{
    other.colliders = nullptr;
    other.colliderCount = 0;
    other.rotation = 0.f;
    other.needsRotationUpdate = false;
}

CompoundCollider& CompoundCollider::operator=(const CompoundCollider& other)
{
    if (this != &other)
    {
        delete[] colliders;
        colliders = new Collider[MAX_COLLIDERS]{ 0 };
        colliderCount = other.colliderCount;
        for (int i = 0; i < other.colliderCount; ++i)
        {
            memcpy(&colliders[i], &other.colliders[i], sizeof(Collider));
        }
        rotation = other.rotation;
        needsRotationUpdate = other.needsRotationUpdate;
    }
    return *this;
}

CompoundCollider& CompoundCollider::operator=(CompoundCollider&& other) noexcept
{
    if (this != &other)
    {
        delete[] colliders;
        colliders = other.colliders;
        colliderCount = other.colliderCount;
        rotation = other.rotation;
        needsRotationUpdate = other.needsRotationUpdate;
        other.colliders = nullptr;
        other.colliderCount = 0;
        other.rotation = 0.f;
        other.needsRotationUpdate = false;
    }
    return *this;
}

CompoundCollider::~CompoundCollider()
{
    delete[] colliders;
}



bool CompoundCollider::loadColliders(const char* filepath)
{

    std::ifstream file(filepath);
    if (!file.is_open())
        return false;

    std::string line;
    int current = -1;
    colliderCount = 0;

    while (std::getline(file, line))
    {
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
            ss >> key >> eq >> colliderCount;

            if (colliderCount <= 0 || colliderCount > MAX_COLLIDERS)
                return false;

            continue;
        }

        // Collider block header
        if (line[first] == '[')
        {
#if defined(_MSC_VER)
            if (sscanf_s(line.c_str(), "[collider%d]", &current) != 1)
#else
            if (sscanf(line.c_str(), "[collider%d]", &current) != 1)
#endif
                return false;

            if (current < 0 || current >= colliderCount)
                return false;

            colliders[current] = {0};
            continue;
        }

        if (current < 0)
            continue;

        Collider& col = colliders[current];

        // Key-value parsing
        std::stringstream ss(line.substr(first));
        std::string key, eq;
        ss >> key;
        if (key == "count")
        {
            ss >> eq >> col.count;
            if (col.count > MAX_VERTICES) 
            {
                col.count = MAX_VERTICES;
            }
        }
        if (key == "vertices")
        {
            ss >> eq; // remove equal sign
            char l, c, r;
            float x, y;
            unsigned char i = 0;
            while (ss >> l >> x >> c >> y >> r) 
            {
                if (i > col.count - 1) 
                {
                    break;
                }
                col.verts[i] = Vector2{ x, y };
                i++;
            }
        }
        
        else if (key == "radius")
        {
            std::string r;
            ss >> eq >> r;
            if (r == std::string("auto")) {
                col.radius = 0;
            }
            else
            {
                col.radius = std::stof(r);
            }
            
        }
        else if (key == "offset")
        {
            char l, c, r;
            float x, y;
            ss >> eq >> l >> x >> c >> y >> r;
            col.offset = Vector2{ x, y };
        }
    }
    file.close();

    // Final validation
    for (int i = 0; i < colliderCount; ++i)
    {
        Collider& col = colliders[i];

        if (col.radius == 0) {
            bool validBounds = updateBounds(col); // ensure the radius is at least the minimum size of the bounding circle for the collider
            if (!validBounds)
                std::cout << "No radius or vertices specified for collider: " << i << ", in file: " << filepath << "." << std::endl;
        }
    }

    return true;
}

// namespace GJK {
//     bool collided(const CompoundCollider& left, const Vector2 pos1, const CompoundCollider& right, const Vector2 pos2)// uses bound-phase check + GJK algorithm to maximise efficiency
//     {

//         for (int i{ 0 }; i < left.colliderCount; i++) {
//             for (int j{ 0 }; j < right.colliderCount; j++) {
//                 if (!withinBounds(left.colliders[i], pos1, right.colliders[j], pos2)) {
//                     continue;
//                 }
//                 if (left.colliders[i].count == 0 && right.colliders[j].count == 0) {
//                     return true; // if both are circles and passed the bounds check, they have collided
//                 }

//                 Vector2 simplex[3] = {0};
//                 if (gjk(left.colliders[i], pos1, right.colliders[j], pos2, simplex)) {
//                     return true;
//                 }
//             }
//         }

//         return false;
//     }
// }

void CompoundCollider::drawDebug(const Vector2& pos, const Color c) const
{
    for (int i{ 0 }; i < colliderCount; i++) {
        
        const Collider& col = colliders[i]; 
        Vector2 origin = Vector2Add(pos, col.offset);
        if (col.count == 0) {
            DrawCircleLines((int)roundf(origin.x), (int)roundf(origin.y), col.radius, c);
            continue;
        }
        for (unsigned char v = 0; v < col.count - 1; v++) {
            DrawLine((int)roundf(col.verts[v].x + origin.x), (int)roundf(col.verts[v].y + origin.y), (int)roundf(col.verts[v + 1].x + origin.x), (int)roundf(col.verts[v + 1].y + origin.y), c);
        }
        DrawLine((int)roundf(col.verts[0].x + origin.x), (int)roundf(col.verts[0].y + origin.y), (int)roundf(col.verts[col.count - 1].x + origin.x), (int)roundf(col.verts[col.count - 1].y + origin.y), c);
    }
    
}

void CompoundCollider::setRotation(float rot)
{
    rotation = rot;
    needsRotationUpdate = true;
}

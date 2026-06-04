#pragma once
#include <cstdint>
#include "core/EntityID.h"
#include "world/ChunkCoord.h"
#include "raylib.h"


constexpr uint16_t MAX_PROJ = 1000;
constexpr uint16_t MAX_HITS_PER_FRAME = 1000;
const uint16_t MAX_CHUNKS_PER_RAY = 10;

enum class ProjectileType : uint8_t {
    Laser = 0
};

struct ProjectilePool {

    float posX[MAX_PROJ];
    float posY[MAX_PROJ];
    float velX[MAX_PROJ];
    float velY[MAX_PROJ];
    float lifetime[MAX_PROJ];
    bool  active[MAX_PROJ];
    ChunkCoord chunk[MAX_PROJ];

    int16_t damage[MAX_PROJ];
    EntityID ownerID[MAX_PROJ];
    ProjectileType type[MAX_PROJ];

    uint16_t count = 0;
    uint16_t next = 0;
};

struct HitEvent {
    uint16_t projectileIndex;
    EntityID hitEntity;
    Vector2  contactPoint;
    Vector2  contactNormal;
    int16_t  damage;
};

struct HitEventBuffer {
    HitEvent events[MAX_HITS_PER_FRAME];
    int count = 0;

    void push(HitEvent e) {
        if (count < MAX_HITS_PER_FRAME)
            events[count++] = e;
    }

    void flush() { count = 0; }
};

namespace Core::Projectiles {
    extern ProjectilePool pool;
    extern HitEventBuffer hb;

    void resolveChunkBoundary(uint16_t id);
    void update(float dt);
    // void updateCollisionsProjectiles(ProjectilePool& pool, HitEventBuffer& hitBuffer);
    // void collison(float dt);
    void spawn(float x, float y, float vx, float vy, float lifetime, ChunkCoord coord, int16_t damage, EntityID ownerID, ProjectileType type);
    void renderProjectiles();
    void debugRender();
}
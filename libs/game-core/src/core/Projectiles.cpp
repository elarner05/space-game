#include "core/Projectiles.h"
#include "core/Core.h" // chunkmap required
#include "components/Kinematics.h" // only imported for chunk sizes
#include "world/GameCamera.h"

#include <cassert>

namespace Core::Projectiles {
    ProjectilePool pool;
    HitEventBuffer hb;
}

void Core::Projectiles::resolveChunkBoundary(uint16_t i) {
    while (pool.posX[i] >= CHUNK_SIZEF) { pool.posX[i] -= CHUNK_SIZEF; pool.chunk[i].x++; }
    while (pool.posX[i] <  0.f)         { pool.posX[i] += CHUNK_SIZEF; pool.chunk[i].x--; }
    while (pool.posY[i] >= CHUNK_SIZEF) { pool.posY[i] -= CHUNK_SIZEF; pool.chunk[i].y++; }
    while (pool.posY[i] <  0.f)         { pool.posY[i] += CHUNK_SIZEF; pool.chunk[i].y--; }
}


// Position of target expressed in projectile chunk local space
inline Vector2 toRelative(ChunkCoord projChunk, ChunkCoord targetChunk, Vector2 targetLocal) {
    return {
        (targetChunk.x - projChunk.x) * CHUNK_SIZEF + targetLocal.x,
        (targetChunk.y - projChunk.y) * CHUNK_SIZEF + targetLocal.y
    };
}

// DDA - collect every chunk the ray [a -> b] (projChunk-local) passes through
// a and b may span multiple chunks; returns absolute ChunkCoords
inline int collectChunksAlongRay(ChunkCoord  projChunk, Vector2 rayStart, Vector2 rayEnd, ChunkCoord* out, int maxOut) {
    // Convert endpoints to absolute chunk coords
    auto toAbsChunk = [&](Vector2 p) -> ChunkCoord {
        int cx = projChunk.x + (int)floorf(p.x / CHUNK_SIZEF);
        int cy = projChunk.y + (int)floorf(p.y / CHUNK_SIZEF);
        return { cx, cy };
    };

    ChunkCoord start = toAbsChunk(rayStart);
    ChunkCoord end   = toAbsChunk(rayEnd);

    int x = start.x, y = start.y;
    int stepX = (end.x > start.x) ? 1 : (end.x < start.x) ? -1 : 0;
    int stepY = (end.y > start.y) ? 1 : (end.y < start.y) ? -1 : 0;

    int count = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            out[count++] = { x + dx, y + dy };
        }
    }

    // DDA - track t at which ray crosses each grid line
    float rayDX = rayEnd.x - rayStart.x;
    float rayDY = rayEnd.y - rayStart.y;

    float tDeltaX = (fabsf(rayDX) > 1e-6f) ? fabsf(CHUNK_SIZEF / rayDX) : 1e30f;
    float tDeltaY = (fabsf(rayDY) > 1e-6f) ? fabsf(CHUNK_SIZEF / rayDY) : 1e30f;

    // t at first vertical / horizontal crossing
    float tMaxX, tMaxY;
    if (stepX > 0)
        tMaxX = ((start.x - projChunk.x + 1) * CHUNK_SIZEF - rayStart.x) / rayDX;
    else if (stepX < 0)
        tMaxX = ((start.x - projChunk.x)     * CHUNK_SIZEF - rayStart.x) / rayDX;
    else
        tMaxX = 1e30f;

    if (stepY > 0)
        tMaxY = ((start.y - projChunk.y + 1) * CHUNK_SIZEF - rayStart.y) / rayDY;
    else if (stepY < 0)
        tMaxY = ((start.y - projChunk.y)     * CHUNK_SIZEF - rayStart.y) / rayDY;
    else
        tMaxY = 1e30f;

    while ((x != end.x || y != end.y) && count < maxOut - 1) {
        if (tMaxX < tMaxY) {
            x += stepX;
            tMaxX += tDeltaX;
        }
        else if (tMaxY < tMaxX) {
            y += stepY;
            tMaxY += tDeltaY;
        }
        else {
            x += stepX;
            y += stepY;
            tMaxX += tDeltaX;
            tMaxY += tDeltaY;
        }
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                out[count++] = { x + dx, y + dy };
            }
        }
    }

    return count;
}

// ray vs circle - returns t [0,1] of first intersection, or -1
inline float rayVsCircle(Vector2 a, Vector2 b, Vector2 center, float radius) {
    float dx = b.x - a.x,  dy = b.y - a.y;
    float fx = a.x - center.x, fy = a.y - center.y;

    float A = dx*dx + dy*dy;
    float B = 2.f * (fx*dx + fy*dy);
    float C = fx*fx + fy*fy - radius*radius;
    if (C <= 0.f)
        return 0.f;

    float disc = B*B - 4.f*A*C;
    if (disc < 0.f) return -1.f;

    float sqrtDisc = sqrtf(disc);
    float t = (-B - sqrtDisc) / (2.f * A);
    if (t >= 0.f && t <= 1.f) return t;
    t = (-B + sqrtDisc) / (2.f * A);
    if (t >= 0.f && t <= 1.f) return t;
    return -1.f;
}

// ray vs convex polygon - returns t [0,1] of entry, or -1
// Uses slab method: find t range where ray is inside all half-planes
inline float rayVsConvex(Vector2 rayStart, Vector2 rayEnd, const Collider& col, Vector2 entityPos) {
    float tEnter = 0.f;
    float tExit  = 1.f;

    float rdx = rayEnd.x - rayStart.x;
    float rdy = rayEnd.y - rayStart.y;

    for (int i = 0; i < (int)col.count; i++) {
        Vector2 a = { col.verts[i].x               + entityPos.x + col.offset.x,
                      col.verts[i].y               + entityPos.y + col.offset.y };
        Vector2 b = { col.verts[(i+1) % col.count].x + entityPos.x + col.offset.x,
                      col.verts[(i+1) % col.count].y + entityPos.y + col.offset.y };

        float ex = b.x - a.x, ey = b.y - a.y;
        float nx = ey, ny = -ex; // outward normals (CCW Y-down)

        float denom = nx * rdx + ny * rdy;  // how fast ray approaches plane
        float dist  = nx * (rayStart.x - a.x) + ny * (rayStart.y - a.y); // signed dist of start from plane

        if (fabsf(denom) < 1e-6f) {
            // ray parallel to edge - if outside this edge, no hit possible
            if (dist < 0.f) return -1.f;
            continue;
        }

        float t = -dist / denom;

        if (denom < 0.f) tEnter = fmaxf(tEnter, t); // entering half-plane
        else             tExit  = fminf(tExit,  t);  // exiting half-plane

        if (tEnter > tExit) return -1.f; // missed
    }

    return tEnter; // first moment ray is inside all half-planes
}


// Outward-facing normal of the edge the point is closest to
inline Vector2 closestEdgeNormal(const Collider& col, Vector2 entityPos, Vector2 point) {
    if (col.count == 0) {
        Vector2 center = {
            entityPos.x + col.offset.x,
            entityPos.y + col.offset.y
        };
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        float len = sqrtf(dx*dx + dy*dy);
        if (len < 1e-6f) return { 0.f, -1.f }; // degenerate fallback
        return { dx / len, dy / len };
    }
    float bestDist = 1e30f;
    Vector2 bestNormal = { 0.f, -1.f };

    for (int i = 0; i < (int)col.count; i++) {
        Vector2 a = { col.verts[i].x + entityPos.x + col.offset.x,
                      col.verts[i].y + entityPos.y + col.offset.y};
        Vector2 b = { col.verts[(i+1) % col.count].x + entityPos.x + col.offset.x,
                      col.verts[(i+1) % col.count].y + entityPos.y + col.offset.y};

        // Distance from point to segment
        float ex = b.x - a.x, ey = b.y - a.y;
        float len2 = ex*ex + ey*ey;
        float t = ((point.x - a.x)*ex + (point.y - a.y)*ey) / len2;
        t = fmaxf(0.f, fminf(1.f, t)); // clamp

        float cx = a.x + t*ex - point.x;
        float cy = a.y + t*ey - point.y;
        float dist = cx*cx + cy*cy;

        if (dist < bestDist) {
            bestDist = dist;
            float len = sqrtf(ex*ex + ey*ey);
            bestNormal = { ey / len, -ex / len }; // perpendicular, outward
        }
    }
    return bestNormal;
}

// ray vs one entity - all in projectile chunk local space
// entityRelativePos: entity's Kinematics position expressed relative to projChunk
// outT: [0,1] along ray, outNormal: contact normal

inline bool rayVsEntity(Vector2 rayStart, Vector2 rayEnd, EntityID eid, Vector2 entityRelativePos, float& outT, Vector2& outNormal) {
    const CompoundCollider& cc = Core::getCollider(eid);
    float bestT = 2.f;
    Vector2 bestNormal = {};

    for (int i = 0; i < cc.colliderCount; i++) {
        const Collider& col = cc.colliders[i];

        Vector2 center = {
            entityRelativePos.x + col.offset.x,
            entityRelativePos.y + col.offset.y
        };

        // Broadphase - cheap circle reject
        float tCircle = rayVsCircle(rayStart, rayEnd, center, col.radius);
        if (tCircle < 0.f || tCircle >= bestT) continue;

        // Narrowphase
        float t;
        if (col.count == 0) {
            // Circle collider - bounding circle IS the shape
            t = tCircle;
        } else {
            // Polygon - find actual entry point
            t = rayVsConvex(rayStart, rayEnd, col, entityRelativePos);
            if (t < 0.f || t >= bestT) continue;
        }

        bestT      = t;
        bestNormal = closestEdgeNormal(col, entityRelativePos, {
            rayStart.x + (rayEnd.x - rayStart.x) * t,
            rayStart.y + (rayEnd.y - rayStart.y) * t
        });
    }

    if (bestT > 1.f) return false;
    outT      = bestT;
    outNormal = bestNormal;
    return true;
}

void Core::Projectiles::update(float dt) {
    HitEventBuffer& hitBuffer = hb;
    for (uint16_t i = 0; i < MAX_PROJ; i++) {
        if (!pool.active[i]) continue;

        pool.lifetime[i] -= dt;
        if (pool.lifetime[i] <= 0.f) {
            pool.active[i] = false;
            continue;
        }

        // ray start is current position (projChunk-local)
        Vector2 rayStart = { pool.posX[i], pool.posY[i] };

        // Candidate new position - may cross chunk boundaries
        Vector2 rayEnd = {
            pool.posX[i] + pool.velX[i] * dt,
            pool.posY[i] + pool.velY[i] * dt
        };

        // Collision pass - ray not committed
        float bestT = 2.f;
        EntityID bestEntity = INVALID_ENTITY;
        Vector2 bestNormal = {};
        Vector2 bestHit = {};

        ChunkCoord chunksBuffer[MAX_CHUNKS_PER_RAY];
        int chunkCount = collectChunksAlongRay(pool.chunk[i], rayStart, rayEnd, chunksBuffer, MAX_CHUNKS_PER_RAY);

        for (int c = 0; c < chunkCount; c++) {
            auto it = chunkMap.find(chunksBuffer[c]);
            if (it == chunkMap.end()) continue;

            for (EntityID eid : it->second) {
                if (eid == pool.ownerID[i]) continue; // projectile cannot hit owner; may need teams, or friendly fire

                Vector2 entityRelPos = toRelative(pool.chunk[i], chunksBuffer[c], Core::getKinematics(eid).localPosition); // relative position in projectile chunk space

                float t;
                Vector2 normal;
                if (rayVsEntity(rayStart, rayEnd, eid, entityRelPos, t, normal)) {
                    if (t < bestT) {
                        bestT = t;
                        bestEntity = eid;
                        bestNormal = normal;
                        bestHit = {
                            rayStart.x + (rayEnd.x - rayStart.x) * t,
                            rayStart.y + (rayEnd.y - rayStart.y) * t
                        };
                    }
                }
            }
        }

        if (bestEntity != INVALID_ENTITY) {
            hitBuffer.push(HitEvent{ i, bestEntity, bestHit, bestNormal, pool.damage[i] });
            pool.active[i] = false;
            pool.count-=1;
            pool.next = pool.next>i ? i : pool.next;
            continue; // don't commit position
        }

        // No hit - commit new position and resolve chunk boundary
        pool.posX[i] = rayEnd.x;
        pool.posY[i] = rayEnd.y;
        resolveChunkBoundary(i);
    }
}

void Core::Projectiles::spawn(float x, float y, float vx, float vy, float lifetime, ChunkCoord coord, int16_t damage, EntityID ownerID, ProjectileType type) {
    if (pool.next >= MAX_PROJ) {
        assert(false && "Projectile pool exhausted!"); return;
    }
    pool.count++;
    uint16_t id = pool.next;
    pool.posX[id] = x;
    pool.posY[id] = y;
    pool.velX[id] = vx;
    pool.velY[id] = vy;
    pool.lifetime[id] = lifetime;
    pool.active[id] = true;
    pool.chunk[id] = coord;
    pool.damage[id] = damage;
    pool.ownerID[id] = ownerID;
    pool.type[id] = type;
    while (pool.next < MAX_PROJ && pool.active[pool.next]) {
        pool.next++;
    }
}

void Core::Projectiles::renderProjectiles() {
    for (uint16_t p = 0; p<pool.count; p++) {
        if (!pool.active[p]) continue;
        Vector2 pos = Core::camera.toScreen(pool.posX[p], pool.posY[p], pool.chunk[p]);
        Vector2 normalVel = Vector2Normalize({pool.velX[p], pool.velY[p]});
        DrawLine(pos.x - normalVel.x * 10, pos.y - normalVel.y * 10, pos.x + normalVel.x * 10, pos.y + normalVel.y * 10, GREEN);
    }
}

// debug function
void Core::Projectiles::debugRender() {
    // Draw all active projectile rays this frame
    // very inefficient
    for (uint16_t i = 0; i < pool.count; i++) {
        if (!pool.active[i]) continue;

        Vector2 start = Core::camera.toScreen(pool.posX[i], pool.posY[i], pool.chunk[i]);

        // Compute where rayEnd would be (without dt, just show current vel direction)
        float fakedt = 0.01f;
        float endLocalX = pool.posX[i] + pool.velX[i] * fakedt;
        float endLocalY = pool.posY[i] + pool.velY[i] * fakedt;
        // rough screen pos for end (ignoring chunk boundary for visualisation)
        Vector2 end = Core::camera.toScreen(endLocalX, endLocalY, pool.chunk[i]);

        DrawLineV(start, end, YELLOW);
        DrawCircleV(start, 3.f, YELLOW);
    }

    // Draw all colliders in the chunkmap near any active projectile
    for (uint16_t i = 0; i < pool.count; i++) {
        if (!pool.active[i]) continue;

        ChunkCoord pc = pool.chunk[i];
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                ChunkCoord nc = { pc.x + dx, pc.y + dy };
                auto it = chunkMap.find(nc);
                if (it == chunkMap.end()) continue;

                for (EntityID eid : it->second) {
                    const CompoundCollider& cc = Core::getCollider(eid);
                    Vector2 epos = Core::getKinematics(eid).localPosition;

                    for (int c = 0; c < cc.colliderCount; c++) {
                        const Collider& col = cc.colliders[c];

                        Vector2 worldOffset = {
                            epos.x + col.offset.x,
                            epos.y + col.offset.y
                        };
                        Vector2 screenCenter = Core::camera.toScreen(worldOffset.x, worldOffset.y, nc);

                        if (col.count == 0) {
                            // Circle collider
                            // NOTE: scale radius by zoom if your camera has one
                            DrawCircleLines(screenCenter.x, screenCenter.y, col.radius * Core::camera.renderZoom, BLUE);                        
                        } else {
                            // Polygon - draw each edge
                            for (int v = 0; v < (int)col.count; v++) {
                                Vector2 va = {
                                    epos.x + col.offset.x + col.verts[v].x,
                                    epos.y + col.offset.y + col.verts[v].y
                                };
                                Vector2 vb = {
                                    epos.x + col.offset.x + col.verts[(v+1) % col.count].x,
                                    epos.y + col.offset.y + col.verts[(v+1) % col.count].y
                                };
                                Vector2 sa = Core::camera.toScreen(va.x, va.y, nc);
                                Vector2 sb = Core::camera.toScreen(vb.x, vb.y, nc);
                                DrawLineV(sa, sb, BLUE);

                                // Draw edge normal so we can verify winding
                                float ex = vb.x - va.x, ey = vb.y - va.y;
                                float len = sqrtf(ex*ex + ey*ey);
                                float nx = ey/len, ny = -ex/len; // CCW Y-down outward normal
                                Vector2 mid = Core::camera.toScreen(
                                    (va.x + vb.x) * 0.5f,
                                    (va.y + vb.y) * 0.5f, nc);
                                // Draw outward normal (flip inward)
                                DrawLineV(mid, { mid.x - nx*10.f, mid.y - ny*10.f }, RED);
                            }
                        }
                    }
                }
            }
        }
    }
}
#include "world/GameCamera.h"
#include "core/Core.h"
#include "core/EntityID.h"

// could add some handling for invalid follow targets

GameCamera::GameCamera() : kinematics(), follow(EntityID{0}), currentChunk(kinematics.chunk) {
}
GameCamera::GameCamera(ChunkCoord c, Vector2 pos) : kinematics{c, pos, {0, 0}, 0, 0, 0} ,follow(EntityID{0}), currentChunk(kinematics.chunk) {
}

GameCamera::~GameCamera() {
}

void GameCamera::updatePosition(float dt) {
    const Kinematics& target = Core::getKinematics(follow);
    kinematics.chunk = target.chunk;
    kinematics.localPosition = target.localPosition;
    kinematics.resolveChunk();
    Core::chunkLoader.requireUpdate();
}

Vector2 GameCamera::toScreen(const Kinematics& kin) const {
    float hw = GetScreenWidth()  * 0.5f;
    float hh = GetScreenHeight() * 0.5f;
    // compute offset from camera center in world pixels
    float dx = (float)(kin.chunk.x - kinematics.chunk.x) * CHUNK_SIZEF 
               + kin.localPosition.x - kinematics.localPosition.x;
    float dy = (float)(kin.chunk.y - kinematics.chunk.y) * CHUNK_SIZEF 
               + kin.localPosition.y - kinematics.localPosition.y;
    return {
        dx * renderZoom + hw,
        dy * renderZoom + hh
    };
}

void GameCamera::setFollow(EntityID target) {
    follow = target;
}
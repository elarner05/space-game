#include "GameCamera.h"
GameCamera::GameCamera() : kinematics(), follow(nullptr) {
}
GameCamera::GameCamera(ChunkCoord c, Vector2 pos) : kinematics{c, pos, {0, 0}, 0, 0, 0} ,follow(nullptr) {
}

GameCamera::~GameCamera() {
}

void GameCamera::updatePosition(float dt) {
    // could have some nice follow the object code here
    // for now just directly set to follow target
    if (follow == nullptr) return; 
    kinematics.chunk = follow->chunk;
    kinematics.localPosition = follow->localPosition;
    kinematics.localPosition = Vector2Subtract(kinematics.localPosition, Vector2{ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f });
    bool changed = kinematics.resolveChunk();

}

Vector2 GameCamera::toScreen(const Kinematics& kin) const {
    return kin.localPositionRelativeTo(kinematics.chunk, kinematics.localPosition);
}

void GameCamera::setFollow(const Kinematics* target) {
    follow = target;
}
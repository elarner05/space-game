#include "GameCamera.h"
#include "Core.h"
#include "EntityID.h"

// need to add some handling for invalid follow targets

GameCamera::GameCamera() : kinematics(), follow(EntityID{0}) {
}
GameCamera::GameCamera(ChunkCoord c, Vector2 pos) : kinematics{c, pos, {0, 0}, 0, 0, 0} ,follow(EntityID{0}), currentChunk(c) {
}

GameCamera::~GameCamera() {
}

void GameCamera::updatePosition(float dt) {
    // could have some nice follow the object code here
    // for now just directly set to follow target
    currentChunk = Core::getKinematics(follow).chunk;

    kinematics.chunk = Core::getKinematics(follow).chunk;
    kinematics.localPosition = Core::getKinematics(follow).localPosition;
    kinematics.localPosition = Vector2Subtract(kinematics.localPosition, Vector2{ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f });
    bool changed = kinematics.resolveChunk();

}

Vector2 GameCamera::toScreen(const Kinematics& kin) const {
    return kin.localPositionRelativeTo(kinematics.chunk, kinematics.localPosition);
}

void GameCamera::setFollow(EntityID target) {
    follow = target;
}
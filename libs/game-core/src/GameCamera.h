#pragma once
#include "Kinematics.h"
// #include "Core.h"
#include "EntityID.h"
class GameCamera {
public:
    Kinematics kinematics; // camera has a kinematic component, can be accelerated etc
    EntityID follow;

    constexpr static int simulationDistance = 3;
    constexpr static int loadDistance = simulationDistance + 2;

    GameCamera();
    GameCamera(ChunkCoord c, Vector2 pos);
    ~GameCamera();

    void setFollow(EntityID target);
    void updatePosition(float dt);
    Vector2 toScreen(const Kinematics& kin) const;
};
#pragma once
#include "components/Kinematics.h"
// #include "Core.h"
#include "core/EntityID.h"
class GameCamera {
public:
    Kinematics kinematics;  // camera has a kinematic component, can be accelerated etc
                            // stores upper left hand corner position

    ChunkCoord currentChunk;// stores chunk of middle of the screen (used for loading etc)
    EntityID follow;

    constexpr static int simulationDistance = 3;
    constexpr static int loadDistance = 3;

    GameCamera();
    GameCamera(ChunkCoord c, Vector2 pos);
    ~GameCamera();

    void setFollow(EntityID target);
    void updatePosition(float dt);
    Vector2 toScreen(const Kinematics& kin) const;
};
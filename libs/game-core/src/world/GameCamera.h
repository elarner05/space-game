#pragma once
#include "components/Kinematics.h"
// #include "Core.h"
#include "core/EntityID.h"

class GameCamera {
public:
    enum class Mode {
        Follow,
        Free
    };
    Kinematics kinematics;  // camera has a kinematic component, can be accelerated etc
                            // stores center of the screen

    ChunkCoord& currentChunk; // stores chunk of middle of the screen (used for loading etc)
    EntityID follow;

    Mode mode;

    constexpr static int simulationDistance = 3;
    constexpr static int loadDistance = 3;
    float renderZoom = 1.0f;

    GameCamera();
    GameCamera(ChunkCoord c, Vector2 pos);
    ~GameCamera();

    void setFollow(EntityID target);
    void setFree();

    void changePosition(Vector2 delta);
    void updatePosition(float dt);

    Vector2 toScreen(const Kinematics& kin) const;
    Vector2 toScreen(float posX, float posY, ChunkCoord chunk) const;
};

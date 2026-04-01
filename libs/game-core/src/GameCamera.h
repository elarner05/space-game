#include "Kinematics.h"
class GameCamera {
public:
    Kinematics kinematics; // camera has a kinematic component, can be accelerated etc
    const Kinematics* follow;

    GameCamera();
    GameCamera(ChunkCoord c, Vector2 pos);
    ~GameCamera();

    void setFollow(const Kinematics* target);
    void updatePosition(float dt);
    Vector2 toScreen(const Kinematics& kin) const;
};
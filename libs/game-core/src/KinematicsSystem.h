#pragma once
#include "System.h"
#include "Kinematics.h"

class KinematicsSystem : public System<Kinematics> {
public:
    KinematicsSystem();
    ~KinematicsSystem();

    void update(float dt) override;
    Kinematics* registerEntity(Kinematics entity) override;

    std::deque<Vector2> getPositions() const;
};

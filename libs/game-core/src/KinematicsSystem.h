#pragma once
#include "System.h"
#include "Kinematics.h"
#include "EntityID.h"

class KinematicsSystem : public System<Kinematics> {
public:
    KinematicsSystem();
    ~KinematicsSystem();

    void update(float dt) override;
    EntityID registerEntity(Kinematics entity) override;

};

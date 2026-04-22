#pragma once
#include "core/System.h"
#include "components/Kinematics.h"
#include "core/EntityID.h"

class KinematicsSystem : public System<Kinematics> {
public:
    KinematicsSystem();
    ~KinematicsSystem();

    void update(float dt) override;
    void registerEntity(Kinematics entity) override;

};

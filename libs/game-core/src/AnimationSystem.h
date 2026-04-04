#pragma once
#include "System.h"
#include "Animation.h"
#include "EntityID.h"

class AnimationSystem : public System<Animations> {
public:
    AnimationSystem();
    ~AnimationSystem();


    void update(float dt) override;
    EntityID registerEntity(Animations entity) override;
};

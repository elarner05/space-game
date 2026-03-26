#pragma once
#include "System.h"
#include "Animation.h"

class AnimationSystem : public System<Animations> {
public:
    AnimationSystem();
    ~AnimationSystem();


    void update(float dt) override;
    Animations* registerEntity(Animations entity) override;
};

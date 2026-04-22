#pragma once
#include "core/System.h"
#include "components/Animation.h"
#include "core/EntityID.h"

class AnimationSystem : public System<Animations> {
public:
    AnimationSystem();
    ~AnimationSystem();


    void update(float dt) override;
    void registerEntity(Animations entity) override;
};

#pragma once
#include "System.h"
#include "raylib.h"

class RenderSystem : public System<Texture2D> {
public:
    RenderSystem();
    ~RenderSystem();


    void update(float dt) override;
    EntityID registerEntity(Texture2D entity) override;
};

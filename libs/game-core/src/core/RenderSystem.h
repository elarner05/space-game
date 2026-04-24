#pragma once
#include "core/System.h"
#include "raylib.h"

class RenderSystem : public System<Texture2D> {
public:
    RenderSystem();
    ~RenderSystem();


    void update(float dt) override;
    void registerEntity(Texture2D entity) override;
};

namespace Core {
    void drawEntities();
}

#include "raylib.h"

#include "../include/game.h"
#include "game-core.h"
#include <iostream>


void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(InitialWidth, InitialHeight, "Example");
    SetTargetFPS(60);

    Core::Debug::showHitboxes() = false;
    Core::Debug::showEntityOrigins() = false;
    Core::Debug::showVelocities() = false;
    Core::Debug::showChunkBounds() = true;

    Core::init();
}

void GameCleanup()
{
    // unload data
    Core::unloadAll();
    CloseWindow();
}

bool GameUpdate()
{
    return true;
}

void GameDraw()
{
    
}

int main()
{
    GameInit();

    Spaceship s1{ "data/spaceship-1.png", "data/spaceship-1.meta", "data/spaceship-1-collider.meta"};
    Core::camera.setFollow(s1.kinematics);

    Asteroid asts[] = {
        Asteroid(100.f, 200.f, 1, 1, "data/asteroid.png", "data/asteroid.meta", "data/asteroid-collider.meta"),
        Asteroid(150.f, 250.f, 1, 1, "data/asteroid.png", "data/asteroid.meta", "data/asteroid-collider.meta"),
        Asteroid(200.f, 300.f, 1, 1, "data/asteroid.png", "data/asteroid.meta", "data/asteroid-collider.meta"),
        Asteroid(250.f, 350.f, 1, 1, "data/asteroid.png", "data/asteroid.meta", "data/asteroid-collider.meta"),
        Asteroid(300.f, 400.f, 1, 1, "data/asteroid.png", "data/asteroid.meta", "data/asteroid-collider.meta")
    };

    for (size_t i = 0; i < 5; i++) {
        asts[i].kinematics->angularVelocity = (i%2 == 0? -1:1)*0.1f;
    }

    float t = 0.0f;

    while (!WindowShouldClose())
    {
        t += GetFrameTime();
        if (t > 0.01f) {
            
            

            if (IsKeyDown(KEY_W)) {
                if (!(s1.animations->currentAnimation == std::string("engine-start") || s1.animations->currentAnimation == std::string("engine-on"))) {
                    s1.animations->switchAnimation(std::string("engine-start"));
                    s1.animations->changeAnimation(std::string("engine-on"));
                }
                s1.kinematics->accelerate(t, 50.f);
            }
            else {
                if (s1.animations->currentAnimation == std::string("engine-on") || s1.animations->currentAnimation == std::string("engine-start")) {
                    s1.animations->switchAnimation(std::string("idle"));
                }
            }

            if (IsKeyPressed(KEY_C)) {
                Core::Debug::showHitboxes() = !Core::Debug::showHitboxes();
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();
                s1.accelerateRotation(mouse, t, 10.f);
            }
            
            s1.update(t);
            for (auto& ast : asts) {
                ast.update(t);
            }
            Core::update(t);

            t = 0.f;
        }

        if (!GameUpdate())
            break;

        
        BeginDrawing();
        ClearBackground(BLACK);
        
        Core::draw();

        DrawFPS(10, 10);
        EndDrawing();
    }
    GameCleanup();

    return 0;
}
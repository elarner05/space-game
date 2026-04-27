
#include "raylib.h"

#include "../include/game.h"
#include "game-core.h"
#include <iostream>
#include <vector>


// cd build; cmake .. -DENABLE_TRACY=ON;make clean; make;./space-game

void GameInit()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    // SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    
    InitWindow(InitialWidth, InitialHeight, "space-game");
    // SetTargetFPS(60);

    Core::Debug::showHitboxes() = false;
    Core::Debug::showEntityOrigins() = false;
    Core::Debug::showVelocities() = true;
    Core::Debug::showChunkBounds() = true;
    Core::Debug::showCameraPosition() = true;
    Core::Debug::showChunkLoadingBounds() = true;

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

    EntityID s1;

    GlobalSave save{};
    if (Core::saveManager.hasSave() && Core::saveManager.load(save)) {
        Core::camera.kinematics.chunk         = save.playerChunk;
        Core::camera.kinematics.localPosition = save.playerPosition;
        Core::chunkLoader.update();
        s1 = Core::EntityFactory::spawn(EntityTag::Spaceship,
            Kinematics{save.playerChunk, save.playerPosition, 
                      {0,0}, 20, save.playerRotation, 0, 0}, EntityFlags::Persistent);
    } else {
        Core::chunkLoader.update();
        s1 = Core::EntityFactory::spawn(EntityTag::Spaceship,
            Kinematics{{0,0}, {100,100}, {0,0}, 20, 0, 0, 0}, EntityFlags::Persistent);

        // test asteroids
        std::vector<EntityID> asts;
        asts.reserve(1000);

        for (int i = 0; i < 1000; i++) {
            float x = 100.f + (i % 10) * 100.f;
            float y = 200.f + (i / 10) * 100.f;
            float vx = (i % 2 == 0) ? 0.f : -0.f;
            float vy = (i % 3 == 0) ? 0.f : -0.f;

            asts.emplace_back(Core::EntityFactory::spawn(EntityTag::Asteroid, Kinematics{{0, 0}, Vector2{x, y}, Vector2{vx, vy}, 10.f, 0.f, 0.f, 0.f}));
        }

        for (size_t i = 0; i < asts.size(); i++)
            Core::getKinematics(asts[i]).angularVelocity = ((i % 2) == 0 ? -1.f : 1.f) * 0.1f;
    }
    Core::camera.setFollow(s1);

    float t = 0.0f;

    int x = GetScreenHeight();
    int y = GetScreenWidth();

    while (!WindowShouldClose())
    {
        ZoneScopedN("loop");
        {
        ZoneScopedN("game_update");
        
        t += GetFrameTime();
        if (t > 0.01f) {
            
            

            if (IsKeyDown(KEY_W)) {
                if (!(Core::getAnimations(s1).currentAnimation == std::string("engine-start") || Core::getAnimations(s1).currentAnimation == std::string("engine-on"))) {
                    Core::getAnimations(s1).switchAnimation(std::string("engine-start"));
                    Core::getAnimations(s1).changeAnimation(std::string("engine-on"));
                }
                Core::getKinematics(s1).accelerate(t, 50.f);
            }
            else {
                if (Core::getAnimations(s1).currentAnimation == std::string("engine-on") || Core::getAnimations(s1).currentAnimation == std::string("engine-start")) {
                    Core::getAnimations(s1).switchAnimation(std::string("idle"));
                }
            }

            if (IsKeyPressed(KEY_C)) {
                Core::Debug::showHitboxes() = !Core::Debug::showHitboxes();
            }
            
            if (GetScreenHeight()!= y || GetScreenWidth() != x) {
                x = GetScreenWidth();
                y = GetScreenHeight();
                std::cout << "New screen size: " << x << "x" << y << std::endl;
            }

            Core::Input::handleSpaceshipInput(s1, t);
            Core::update(t);

            t = 0.f;
        }
        }

        if (!GameUpdate())
            break;

        {
        ZoneScopedN("render_update");
        BeginDrawing();
        ClearBackground(BLACK);
        
        Core::draw();

        DrawFPS(10, 10);
        EndDrawing();
        }
    }
    save.version = 1;
    Kinematics& kin = Core::getKinematics(s1);
    save.playerChunk    = kin.chunk;
    save.playerPosition = kin.localPosition;
    save.playerRotation = kin.rotation;
    Core::saveManager.save(save);
    GameCleanup();

    return 0;
}
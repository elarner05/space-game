
#include "raylib.h"

#include "../include/game.h"   // an external header in this project
#include "game-core.h"	// an external header in the static lib project


void GameInit()
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(InitialWidth, InitialHeight, "Example");
    SetTargetFPS(60);
    //BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);

    // load data
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
    
    // Test rotation calculations

    Spaceship s1{ "data/spaceship-1.png", "data/spaceship-1.meta", "data/spaceship-1-collider.meta"};
    Asteroid a(200.f, 200.f, 1, 1 , "data/asteroid.png", "data/asteroid.meta", "data/asteroid-collider.meta");
    a.kinematics->angularVelocity = 5;
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
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 mouse = GetMousePosition();
                s1.accelerateRotation(mouse, t, 15.f);
            }
            
            /*if (IsKeyDown(KEY_A)) {
                s1.kinematics.rotation-=1;
                if (s1.kinematics.rotation < 0) s1.kinematics.rotation = 359;
            }

            if (IsKeyDown(KEY_D)) {
                s1.kinematics.rotation += 1;
                if (s1.kinematics.rotation > 359) s1.kinematics.rotation = 0;
            }*/
            s1.update(t);
            a.update(t);
            Core::update(t);

            t = 0.f;
        }

        if (!GameUpdate())
            break;

        
        BeginDrawing();
        ClearBackground(BLACK);
        a.draw();
        s1.draw();
        //DrawRectangle(0, 100, 100, 1, RED);
        //DrawRectangle(100, 0, 1, 100, RED);
        DrawFPS(10, 10);
        EndDrawing();
    }
    GameCleanup();

    return 0;
}
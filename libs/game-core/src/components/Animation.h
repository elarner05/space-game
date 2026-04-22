#pragma once
#include "raylib.h"

#include "robin_hood.h"
#include <string>

struct animation {
    int sx;
    int sy;
    int frames;
    int fps;
    bool loop;
};

class Animations {
public:
    Animations() = default;
    Animations(const char* filepath);
    Animations(const Animations& other);
    Animations& operator=(const Animations& other);
    ~Animations() = default;

    bool loadAnimations(const char* filepath);

    bool switchAnimation(const std::string& to);
    bool changeAnimation(const std::string& to);
    void update(float dt);

    Rectangle getSource() const;

    robin_hood::unordered_map<std::string, animation> animations; // this needs to be made shared on an all entities basis, not per-entity

    const animation* currentAnim = nullptr;
    const animation* nextAnim = nullptr;

    Vector2 dimensions = { 0.0f, 0.0f };
    Vector2 origin = { 0.0f, 0.0f };

    std::string currentAnimation;
    std::string nextAnimation;

    int frame = 0;
    float tp = 0.0f;
    bool end = false;
};
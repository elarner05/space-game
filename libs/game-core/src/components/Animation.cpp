#include "components/Animation.h"

#include <assert.h>
#include <fstream>
#include <iostream>

Animations::Animations(const char* filepath) {
    loadAnimations(filepath);
}

Animations::Animations(const Animations& other)
    : animations(other.animations),
      dimensions(other.dimensions),
      origin(other.origin),
      currentAnimation(other.currentAnimation),
      nextAnimation(other.nextAnimation),
      frame(other.frame),
      tp(other.tp),
      end(other.end) {

    currentAnim = nullptr;
    nextAnim = nullptr;

    if (!currentAnimation.empty()) {
        auto currentIt = animations.find(currentAnimation);
        if (currentIt != animations.end()) {
            currentAnim = &currentIt->second;
        }
    }

    if (!nextAnimation.empty()) {
        auto nextIt = animations.find(nextAnimation);
        if (nextIt != animations.end()) {
            nextAnim = &nextIt->second;
        }
    }
}

Animations& Animations::operator=(const Animations& other) {
    if (this == &other) {
        return *this;
    }

    animations = other.animations;
    dimensions = other.dimensions;
    origin = other.origin;

    currentAnimation = other.currentAnimation;
    nextAnimation = other.nextAnimation;

    frame = other.frame;
    tp = other.tp;
    end = other.end;

    currentAnim = nullptr;
    nextAnim = nullptr;

    if (!currentAnimation.empty()) {
        auto currentIt = animations.find(currentAnimation);
        if (currentIt != animations.end()) {
            currentAnim = &currentIt->second;
        }
    }

    if (!nextAnimation.empty()) {
        auto nextIt = animations.find(nextAnimation);
        if (nextIt != animations.end()) {
            nextAnim = &nextIt->second;
        }
    }

    return *this;
}

bool Animations::loadAnimations(const char* filepath) {
    std::ifstream fin(filepath);

    if (!fin) {
        std::cerr << "Failed to open animation file: " << filepath << "\n";
        return false;
    }

    animations.clear();
    currentAnim = nullptr;
    nextAnim = nullptr;

    float w, h;
    fin >> w >> h;
    dimensions = Vector2{ w, h };

    float ox, oy;
    fin >> ox >> oy;
    origin = Vector2{ ox, oy };

    std::string name;
    int sx, sy, frames, fps;

    while (fin >> name >> sx >> sy >> frames >> fps) {
        if (frames <= 0) {
            std::cerr << "Animation \"" << name << "\" has invalid frame count\n";
            continue;
        }

        if (fps <= 0) {
            std::cerr << "Animation \"" << name << "\" has invalid fps\n";
            continue;
        }

        animations.insert({
            name,
            animation{
                sx,
                sy,
                frames,
                fps,
                false
            }
        });
    }

    fin.close();

    frame = 0;
    tp = 0.0f;
    end = false;

    currentAnimation.clear();
    nextAnimation.clear();

    auto idleIt = animations.find("idle");
    if (idleIt != animations.end()) {
        currentAnimation = "idle";
        currentAnim = &idleIt->second;
    }

    return true;
}

bool Animations::switchAnimation(const std::string& to) {
	if (currentAnimation == to) {
        return true;
    }
    auto it = animations.find(to);

    if (it == animations.end()) {
        return false;
    }

    currentAnimation = to;
    currentAnim = &it->second;

    nextAnimation.clear();
    nextAnim = nullptr;

    frame = 0;
    tp = 0.0f;
    end = false;

    return true;
}

bool Animations::changeAnimation(const std::string& to) {
    auto it = animations.find(to);

    if (it == animations.end()) {
        return false;
    }

    nextAnimation = to;
    nextAnim = &it->second;
    end = true;

    return true;
}

void Animations::update(float dt) {
    if (currentAnim == nullptr) {
        return;
    }

    assert(currentAnim->fps > 0);
    assert(currentAnim->frames > 0);

    tp += dt;

    const float frameTime = 1.0f / currentAnim->fps;

    while (tp >= frameTime) {
        tp -= frameTime;
        frame++;

        if (frame >= currentAnim->frames) {
            frame = 0;

            if (end && nextAnim != nullptr) {
                currentAnimation = nextAnimation;
                currentAnim = nextAnim;

                nextAnimation.clear();
                nextAnim = nullptr;
                end = false;
            }
        }
    }
}

Rectangle Animations::getSource() const {
    if (currentAnim == nullptr) {
        return Rectangle{ 0.0f, 0.0f, 0.0f, 0.0f };
    }

    return Rectangle{
        (currentAnim->sx + frame) * dimensions.x,
        currentAnim->sy * dimensions.y,
        dimensions.x,
        dimensions.y
    };
}
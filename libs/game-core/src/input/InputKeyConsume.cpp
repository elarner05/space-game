
#include "input/InputKeyConsume.h"
namespace Core::Input::Consume {
    namespace {
        bool consumedKeys[512] = { false };
    }

    bool& consumed(KeyboardKey key) {
        return consumedKeys[key];
    }

    bool pressed(KeyboardKey key) {
        if (consumedKeys[key]) return false;
        if (IsKeyDown(key)) {
            consumedKeys[key] = true;
            return true;
        }
        return false;
    }

    // call at start of each frame
    void update() {
        for (int i = 0; i < 512; i++) {
            if (consumedKeys[i] && !IsKeyDown((KeyboardKey)i)) {
                consumedKeys[i] = false;
            }
        }
    }
}
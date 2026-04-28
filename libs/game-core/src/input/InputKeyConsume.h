#pragma once
#include "raylib.h"

namespace Core::Input::Consume {
    bool& consumed(KeyboardKey key);
    
    // returns true if pressed and not yet consumed, then consumes it
    bool pressed(KeyboardKey key);

    // call once per frame to release consumed keys that have been let go
    void update();
}
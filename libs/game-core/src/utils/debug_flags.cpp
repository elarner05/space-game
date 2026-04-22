#include "utils/debug_flags.h"

namespace Core::Debug {
    bool& showHitboxes() {
        static bool flag = false;
        return flag;
    }
    bool& showEntityOrigins() {
        static bool flag = false;
        return flag;
    }

    bool& showVelocities() {
        static bool flag = false;
        return flag;
    }

    bool& showChunkBounds() {
        static bool flag = false;
        return flag;
    }

    bool& showCameraPosition() {
        static bool flag = false;
        return flag;
    }
}
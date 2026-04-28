#include "core/AnimationSystem.h"
#include "Core.h"

void Core::updateAnimations(float dt) {
    for (uint32_t i = 0; i < animationTable.size(); i++) {
        Animations& anim = animationTable[i];
        anim.update(dt);
    }
}

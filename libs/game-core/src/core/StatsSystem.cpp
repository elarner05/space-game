#include "StatsSystem.h"
#include "core/Projectiles.h"
#include "core/Core.h"
#include "robin_hood.h"
#include <iostream>


static robin_hood::unordered_set<EntityID> deadEntities;

void processHitEvents(HitEventBuffer& hitBuffer) {deadEntities.reserve(1024);
    for (int i = 0; i < hitBuffer.count; i++) {
        const HitEvent& hit = hitBuffer.events[i];

        // guard - entity may have died earlier this frame from another hit
        // if (Core::entityFlagTable[Core::slots[hit.hitEntity.index].arrayIndex] & EntityFlags::Dead) continue;
        Stats& stats =  Core::statsTable[Core::slots[hit.hitEntity.index].arrayIndex];

        bool died = stats.dealDamage(hit.damage);
        std::cout << "Dealt dmg: " << hit.damage << ", Health: " << stats.health << std::endl;
        if (died) {
            deadEntities.insert(hit.hitEntity);
            // Core::entityFlagTable[hit.hitEntity] |= EntityFlags::Dead;
            // queue death effects, drops, etc here
        }

        // spawn hit effect, sounds, etc
        // spawnHitEffect(hit.contactPoint, hit.contactNormal);
    }

    for (EntityID e: deadEntities) {
        Core::unregisterEntity(e);
    }
    deadEntities.clear();

    hitBuffer.flush();
}
void Core::updateStats(float dt) {
    processHitEvents(Core::Projectiles::hb);
}
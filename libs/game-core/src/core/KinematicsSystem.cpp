#include "core/KinematicsSystem.h"
#include "core/Core.h"
#include "core/ChunkMapUtil.h"

void Core::updateKinematics(float dt) {
    for (uint32_t i = 0; i < kinematicsTable.size(); i++) {
        Kinematics& kin = kinematicsTable[i];

        kin.update(dt);

        Core::resolveEntityChunk(kin, Core::indexToEntity[i]);
        Core::getCollider(Core::indexToEntity[i]).setRotation(kin.rotation);
    }
}

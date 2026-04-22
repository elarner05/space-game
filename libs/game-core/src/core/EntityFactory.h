#pragma once
#include "components/Kinematics.h"
#include "core/EntityID.h"
#include "components/EntityTypes.h"

namespace Core {
    namespace EntityFactory {
        EntityID spawn(const EntityTag tag, EntityFlags flags = EntityFlags::None);
        EntityID spawn(const EntityTag tag, Kinematics kin, EntityFlags flags = EntityFlags::None);
    }
}
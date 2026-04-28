#include "core/Core.h"

namespace Core {
    bool resolveEntityChunk(Kinematics& kin, EntityID id);
    bool resolveAllEntityChunks();
    bool setEntityChunk(Kinematics& kin, EntityID id, ChunkCoord newChunk);
}

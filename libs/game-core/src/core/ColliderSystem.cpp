#include "core/ColliderSystem.h"
#include "core/Core.h"

bool checkNeedsRotationUpdate() {
    for (size_t i = 0; i < Core::colliderTable.size(); ++i) {
        if (Core::colliderTable[i].needsRotationUpdate) {
            // std::cout << "Rotation update needed for entity " << i << std::endl;
            return true;
        }
    }
    return false;
}

void applyRotations() {
    for (size_t i = 0; i < Core::colliderTable.size(); ++i) {
        CompoundCollider& col = Core::colliderTable[i];
        if (col.needsRotationUpdate) {
            // apply new rotation to each collider
            for (int c = 0; c < Core::originalColliderTable[i].colliderCount; ++c) {
                Collider& collider = col.colliders[c];
                Collider& originalCol = Core::originalColliderTable[i].colliders[c];

                // rotate offset
                float cosRot = cosf(col.rotation);
                float sinRot = sinf(col.rotation);
                Vector2 rotatedOffset = {
                    originalCol.offset.x * cosRot - originalCol.offset.y * sinRot,
                    originalCol.offset.x * sinRot + originalCol.offset.y * cosRot
                };
                // Keep offset as float - will be rounded at draw time
                collider.offset = rotatedOffset;

                // rotate vertices if not a circle collider
                if (collider.count > 0) {
                    for (unsigned char v = 0; v < collider.count; ++v) {
                        Vector2 vert = originalCol.verts[v];
                        Vector2 rotatedVert = {
                            vert.x * cosRot - vert.y * sinRot,
                            vert.x * sinRot + vert.y * cosRot
                        };
                        collider.verts[v] = {rotatedVert.x, rotatedVert.y};
                    }
                }
            }
            col.needsRotationUpdate = false;
        }
    }
}

void Core::updateColliders(float dt) {
    if (checkNeedsRotationUpdate()) {
        applyRotations();
    }
}

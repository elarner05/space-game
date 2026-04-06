#pragma once
#include <cmath>
#include "robin_hood.h"

struct ChunkCoord{
    int x;
    int y;

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const ChunkCoord& other) const {
        return !(*this == other);
    }

    // proximity check for broad phase
    bool isAdjacentTo(const ChunkCoord& other) const {
        return abs(x - other.x) <= 1 && abs(y - other.y) <= 1;
    }

    int chebyshevDistance(const ChunkCoord& other) const { // might be needed in chunk loading
        return std::max(abs(x - other.x), abs(y - other.y));
    }
};

namespace robin_hood {
    template<>
    struct hash<ChunkCoord> {
        size_t operator()(const ChunkCoord& c) const noexcept {
            size_t h1 = hash<int>{}(c.x);
            size_t h2 = hash<int>{}(c.y);
            return h1 ^ (h2 * 2654435761u);
        }
    };
}
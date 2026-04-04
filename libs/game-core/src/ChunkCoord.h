#pragma once
#include <cmath>

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
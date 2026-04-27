#pragma once
#include "robin_hood.h"

struct EntityID {
    uint32_t index;
    uint32_t generation;
    
    bool operator==(const EntityID& other) const { return index == other.index && generation == other.generation; }
    bool operator!=(const EntityID& other) const { return index != other.index || generation != other.generation; }
    
    static constexpr EntityID invalid() { return EntityID{UINT32_MAX, UINT32_MAX}; }
    bool isValid() const { return index != UINT32_MAX; }

    bool operator>(const EntityID& other) const {return index > other.index;}
    
    bool operator<(const EntityID& other) const {return index < other.index;}
};

struct Slot {
    uint32_t arrayIndex;  // where in the component arrays this entity lives
    uint32_t generation;  // incremented each time this slot is reused
};


// hash function
namespace robin_hood {
    template<>
    struct hash<EntityID> {
        size_t operator()(const EntityID& id) const noexcept {
            return hash<uint32_t>{}(id.index);
        }
    };
}
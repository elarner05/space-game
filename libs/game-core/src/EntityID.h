#pragma once
#include "robin_hood.h"

struct EntityID {
    uint32_t value;
    
    bool operator==(const EntityID& other) const { return value == other.value; }
    bool operator!=(const EntityID& other) const { return value != other.value; }
    
    static constexpr EntityID invalid() { return EntityID{UINT32_MAX}; }
    bool isValid() const { return value != UINT32_MAX; }

    
    operator size_t() const {return static_cast<size_t>(value);}// Conversion operator

    bool operator>(const EntityID& other) const {return value > other.value;}
    
    bool operator<(const EntityID& other) const {return value < other.value;}
};

// hash function
namespace robin_hood {
    template<>
    struct hash<EntityID> {
        size_t operator()(const EntityID& id) const noexcept {
            return hash<uint32_t>{}(id.value);
        }
    };
}
#pragma once
#include "EntityID.h"
#include <vector>

template<typename T>
class System {
public:
    std::vector<T> m_entities;

public:

    virtual void update(float dt) = 0;
    virtual void registerEntity(T entity) = 0;
};
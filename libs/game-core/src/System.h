#pragma once

#include <deque>

template<typename T>
class System {
public:
    std::deque<T> m_entities;

public:

    virtual void update(float dt) = 0;
    virtual T* registerEntity(T entity) = 0;
};
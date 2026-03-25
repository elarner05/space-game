#pragma once

class Entity {
public:
	Entity() {};
	~Entity() {};
	virtual void draw() = 0;
	virtual void update(float dt) = 0;
	virtual void reset() = 0;
};
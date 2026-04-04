#pragma once
#include "raylib.h"
#include <map>
#include <string>

struct animation {

	int sx;
	int sy;
	int frames;
	int fps;
	bool loop;
};

class Animations {
public:
	Animations();
	Animations(const char* filepath);
	Animations(const Animations& other);
	~Animations();

	bool loadAnimations(const char* filepath);

	void switchAnimation(std::string to);
	void changeAnimation(std::string to);
	void update(float dt);

	Rectangle getSource() const;

	std::map<std::string, animation> animations;
	Vector2 dimensions;
	Vector2 origin;

	std::string currentAnimation;
	std::string nextAnimation;
	int frame;
	float tp;
	bool end;

};
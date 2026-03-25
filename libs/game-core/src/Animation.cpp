#include "Animation.h"
#include <fstream>
#include <assert.h>
#include <iostream>

Animations::Animations(const char* filepath) {

	std::ifstream fin(filepath);
	std::cout << filepath << std::endl;
	assert(fin && "Failed to open animation file");

	float w, h;
	fin >> w;
	fin >> h;

	dimensions = Vector2{ w, h };

	float ox, oy;
	fin >> ox >> oy;

	origin = Vector2{ ox, oy };

	std::string name;
	int sx, sy, frames, fps;
	while (fin >> name >> sx >> sy >> frames >> fps) {
		//std::cout << name << " " << sx << " " << sy << " " << frames << " " << fps << std::endl;
		animation anim{ sx, sy, frames, fps, false};
		animations.insert({ name, anim });
	}

	fin.close();

	currentAnimation = std::string("idle");
	nextAnimation = std::string("idle");
	frame = 0;
	tp = 0.f; // time passed
	end = false;

}

Animations::~Animations() {

}

void Animations::switchAnimation(std::string to) {
	currentAnimation = to;
	tp = 0;
	frame = 0;
	end = false;
}

void Animations::changeAnimation(std::string to) {
	nextAnimation = to;
	end = true;
}

void Animations::update(float dt) {
	tp += dt;
	auto itr = animations.find(currentAnimation);
	if (itr == animations.end()) {
		return;
	}

	const animation &a = itr->second;

	const float frameTime = 1.0f / a.fps;

	while (tp >= frameTime) {
		tp -= frameTime;
		frame++;

		if (frame >= a.frames) {
			frame = 0;
		}

		if (end && frame == 0) {
			currentAnimation = nextAnimation;
			end = false;
			break;
		}
	}
}

Rectangle Animations::getSource() const {
	auto itr = animations.find(currentAnimation);

	if (itr == animations.end()) {
		return Rectangle{ 0, 0, 0, 0 };
	}
		
	const animation &a = itr->second;
	//std::cout << (a.sx + frame) * dimensions.x<< (a.sy) * dimensions.y<< dimensions.x<< dimensions.y << std::endl;;

	return Rectangle{(a.sx + frame) * dimensions.x, (a.sy) * dimensions.y, dimensions.x, dimensions.y};
}
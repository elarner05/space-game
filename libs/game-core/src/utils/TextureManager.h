#pragma once
#include "raylib.h"
#include <string>

namespace TextureManager {
	Texture2D& loadTexture(std::string filepath);
	void unloadTexture(std::string filepath);
	void unloadAllTextures();
}
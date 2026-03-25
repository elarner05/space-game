#pragma once
#include "raylib.h"
#include <string>

namespace TextureManager {
	Texture2D loadTexture(const char* filepath);
	void unloadTexture(const char* filepath);
	void unloadAllTextures();
}
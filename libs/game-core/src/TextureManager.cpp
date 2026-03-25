#include "TextureManager.h"
#include <map>

namespace TextureManager {
	std::map<const char*, Texture2D> textures;

	Texture2D loadTexture(const char* filepath) {
		auto itr = textures.find(filepath);
		if (itr != textures.end()) {
			return itr->second;
		}
		Texture2D tex = LoadTexture(filepath);
		textures.insert({ filepath, tex });
		return tex;
	}
	void unloadTexture(const char *filepath) {
		auto itr = textures.find(filepath);
		if (itr != textures.end()) {
			UnloadTexture(itr->second);
			textures.erase(itr->first);
		}
	}
	void unloadAllTextures() {
		for (auto itr = textures.begin(); itr != textures.end(); ++itr) {
			UnloadTexture(itr->second);
		}
		textures.clear();
	}
}

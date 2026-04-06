#include "TextureManager.h"
#include "robin_hood.h"

namespace TextureManager {
	robin_hood::unordered_map<std::string, Texture2D> textures;

	Texture2D& loadTexture(std::string filepath) {
		auto itr = textures.find(filepath);
		if (itr != textures.end()) {
			return itr->second;
		}
		Texture2D tex = LoadTexture(filepath.c_str());
		textures.insert({ filepath, tex });
		return textures[filepath];
	}
	void unloadTexture(std::string filepath) {
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

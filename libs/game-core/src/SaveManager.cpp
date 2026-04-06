#include "SaveManager.h"
#include <string>
#include <fstream>
#include <filesystem>

static constexpr uint32_t SAVE_VERSION = 1;

std::string mainSaveDataPath(const std::string& savePath) {
    return savePath + "/save.bin";
}

SaveManager::SaveManager(const std::string& savePath)
    : m_savePath(savePath)
{
    std::filesystem::create_directories(savePath);
}

bool SaveManager::hasSave() const {
    return std::filesystem::exists(mainSaveDataPath(m_savePath));
}

bool SaveManager::load(GlobalSave& out) const {
    std::string path = mainSaveDataPath(m_savePath);
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    file.read(reinterpret_cast<char*>(&out), sizeof(out));
    if (out.version != SAVE_VERSION) return false;
    return true;
}

bool SaveManager::save(const GlobalSave& data) const {
    std::string path = mainSaveDataPath(m_savePath);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);

    if (!file) return false;

    file.write(reinterpret_cast<const char*>(&data), sizeof(data));
    return true;
}
#include "audio/BonkPackManager.h"
#include <filesystem>
#include <fstream>
#include <set>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

std::vector<BonkPackConfig> BonkPackManager::scanForBonkPacks(const std::string& basePath) {
    std::vector<BonkPackConfig> packs;
    std::set<std::string> seenPaths;
    
    // Try to find bonk-packs directory - check multiple possible locations
    std::vector<std::filesystem::path> searchPaths;
    
    // Current directory
    searchPaths.push_back(std::filesystem::path(basePath));
    searchPaths.push_back(std::filesystem::current_path() / basePath);
    
    // Executable directory
    #ifdef _WIN32
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(nullptr, exePath, MAX_PATH) != 0) {
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
        searchPaths.push_back(exeDir / basePath);
        searchPaths.push_back(exeDir.parent_path() / basePath);
    }
    #endif
    
    // Try each search path
    for (const auto& searchPath : searchPaths) {
        if (!std::filesystem::exists(searchPath) || !std::filesystem::is_directory(searchPath)) {
            continue;
        }
        
        // Iterate through subdirectories
        try {
            for (const auto& entry : std::filesystem::directory_iterator(searchPath)) {
                if (entry.is_directory()) {
                    std::string folderPath = std::filesystem::canonical(entry.path()).string();
                    
                    // Skip if we've already seen this folder
                    if (seenPaths.find(folderPath) != seenPaths.end()) {
                        continue;
                    }
                    seenPaths.insert(folderPath);
                    
                    std::string configPath = (entry.path() / "config.txt").string();
                    
                    // Try to load config
                    BonkPackConfig config;
                    if (BonkPackConfig::loadFromFile(configPath, config)) {
                        // Ensure folderPath is set to canonical path
                        try {
                            config.folderPath = std::filesystem::canonical(entry.path()).string();
                        } catch (...) {
                            config.folderPath = entry.path().string();
                        }
                        packs.push_back(config);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Skip directories that can't be accessed
            continue;
        }
    }
    
    // Sort packs by weight (descending), then by name (ascending) for stable ordering
    std::sort(packs.begin(), packs.end(), [](const BonkPackConfig& a, const BonkPackConfig& b) {
        if (a.weight != b.weight) {
            return a.weight > b.weight; // Higher weight first
        }
        return a.name < b.name; // Fallback: alphabetical
    });
    
    return packs;
}

BonkPackConfig BonkPackManager::getDefaultBonkPack() {
    BonkPackConfig config;
    config.name = "None";
    config.bonkSound = "";
    config.iconImage = "";
    config.volume = 100.0f;
    return config;
}

BonkPackConfig BonkPackManager::findBonkPackByName(const std::vector<BonkPackConfig>& packs, const std::string& name) {
    for (const auto& pack : packs) {
        if (pack.name == name) {
            return pack;
        }
    }
    // Return default if not found
    return getDefaultBonkPack();
}


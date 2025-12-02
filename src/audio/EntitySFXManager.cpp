#include "audio/EntitySFXManager.h"
#include <filesystem>
#include <fstream>
#include <set>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

std::vector<EntitySFXConfig> EntitySFXManager::scanForEntitySFX(const std::string& basePath) {
    std::vector<EntitySFXConfig> sfxList;
    std::set<std::string> seenPaths;
    
    // Try to find entity-sfx directory - check multiple possible locations
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
                    EntitySFXConfig config;
                    if (EntitySFXConfig::loadFromFile(configPath, config)) {
                        // Ensure folderPath is set to canonical path
                        try {
                            config.folderPath = std::filesystem::canonical(entry.path()).string();
                        } catch (...) {
                            config.folderPath = entry.path().string();
                        }
                        sfxList.push_back(config);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Skip directories that can't be accessed
            continue;
        }
    }
    
    // Sort by weight (descending), then by name (ascending) for stable ordering
    std::sort(sfxList.begin(), sfxList.end(), [](const EntitySFXConfig& a, const EntitySFXConfig& b) {
        if (a.weight != b.weight) {
            return a.weight > b.weight; // Higher weight first
        }
        return a.name < b.name; // Fallback: alphabetical
    });
    
    return sfxList;
}

EntitySFXConfig EntitySFXManager::getDefaultEntitySFX() {
    EntitySFXConfig config;
    config.name = "None";
    config.entitySound = "";
    config.iconImage = "";
    config.volume = 100.0f;
    return config;
}

EntitySFXConfig EntitySFXManager::findEntitySFXByName(const std::vector<EntitySFXConfig>& sfx, const std::string& name) {
    for (const auto& item : sfx) {
        if (item.name == name) {
            return item;
        }
    }
    // Return default if not found
    return getDefaultEntitySFX();
}


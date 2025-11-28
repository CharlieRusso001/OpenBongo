#include "HatManager.h"
#include <filesystem>
#include <fstream>
#include <set>

#ifdef _WIN32
#include <windows.h>
#endif

std::vector<HatConfig> HatManager::scanForHats(const std::string& basePath) {
    std::vector<HatConfig> hats;
    std::set<std::string> seenPaths; // Track folder paths to avoid duplicates
    
    // Try to find hats directory - check multiple possible locations
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
                    
                    std::string folderName = entry.path().filename().string();
                    std::string configPath = (entry.path() / "config.txt").string();
                    
                    // Try to load config
                    HatConfig config;
                    if (HatConfig::loadFromFile(configPath, config)) {
                        // Ensure folderPath is set to canonical path
                        try {
                            config.folderPath = std::filesystem::canonical(entry.path()).string();
                        } catch (...) {
                            config.folderPath = entry.path().string();
                        }
                        hats.push_back(config);
                    }
                }
            }
        } catch (const std::exception& e) {
            // Skip directories that can't be accessed
            continue;
        }
    }
    
    return hats;
}

HatConfig HatManager::getNoHat() {
    HatConfig config;
    config.name = "No Hat";
    config.hatImage = "";
    config.iconImage = "";
    return config;
}

HatConfig HatManager::findHatByName(const std::vector<HatConfig>& hats, const std::string& name) {
    for (const auto& hat : hats) {
        if (hat.name == name) {
            return hat;
        }
    }
    // Return "no hat" if not found
    return getNoHat();
}


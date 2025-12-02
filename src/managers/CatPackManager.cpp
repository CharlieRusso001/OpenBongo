#include "managers/CatPackManager.h"
#include <filesystem>
#include <fstream>
#include <set>

#ifdef _WIN32
#include <windows.h>
#endif

std::vector<CatPackConfig> CatPackManager::scanForCatPacks(const std::string& basePath) {
    std::vector<CatPackConfig> packs;
    std::set<std::string> seenPaths; // Track folder paths to avoid duplicates
    
    // Try to find catpacks directory - check multiple possible locations
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
                    CatPackConfig config;
                    if (CatPackConfig::loadFromFile(configPath, config)) {
                        // Ensure folderPath is set to canonical path
                        try {
                            config.folderPath = std::filesystem::canonical(entry.path()).string();
                        } catch (...) {
                            config.folderPath = entry.path().string();
                        }
                        packs.push_back(config);
                    } else {
                        // If no config.txt, check for old-style DevArt folder
                        if (folderName == "DevArt") {
                            // Create default config for DevArt
                            config = getDefaultCatPack();
                            try {
                                config.folderPath = std::filesystem::canonical(entry.path()).string();
                            } catch (...) {
                                config.folderPath = entry.path().string();
                            }
                            packs.push_back(config);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            // Skip directories that can't be accessed
            continue;
        }
    }
    
    // Sort packs by weight (descending), then by name (ascending) for stable ordering
    std::sort(packs.begin(), packs.end(), [](const CatPackConfig& a, const CatPackConfig& b) {
        if (a.weight != b.weight) {
            return a.weight > b.weight; // Higher weight first
        }
        return a.name < b.name; // Fallback: alphabetical
    });
    
    return packs;
}

CatPackConfig CatPackManager::getDefaultCatPack() {
    CatPackConfig config;
    config.name = "DevArt Cat";
    config.bodyImage = "body-devartcat.png";
    config.handUpImage = "handup-devartcat.png";
    config.handDownImage = "handdown-devartcat.png";
    config.iconImage = ""; // No icon by default
    // Use default offsets (all 0.0f)
    return config;
}

CatPackConfig CatPackManager::findCatPackByName(const std::vector<CatPackConfig>& packs, const std::string& name) {
    for (const auto& pack : packs) {
        if (pack.name == name) {
            return pack;
        }
    }
    // Return default if not found
    return getDefaultCatPack();
}


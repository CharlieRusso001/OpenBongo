#include "HatConfig.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

bool HatConfig::loadFromFile(const std::string& configPath, HatConfig& config) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }
    
    // Set folder path from config file location
    std::filesystem::path configFile(configPath);
    config.folderPath = configFile.parent_path().string();
    
    std::string line;
    while (std::getline(file, line)) {
        // Remove comments (lines starting with # or //)
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        // Parse key-value pairs
        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) continue;
        
        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);
        
        // Trim key and value
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Convert key to lowercase for case-insensitive matching
        std::string keyLower = key;
        std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
        
        // Parse values
        if (keyLower == "name") {
            config.name = value;
        }
        else if (keyLower == "hatimage" || keyLower == "hat_image") {
            config.hatImage = value;
        }
        else if (keyLower == "iconimage" || keyLower == "icon_image") {
            config.iconImage = value;
        }
        else if (keyLower == "offsetx" || keyLower == "offset_x") {
            config.offsetX = std::stof(value);
        }
        else if (keyLower == "offsety" || keyLower == "offset_y") {
            config.offsetY = std::stof(value);
        }
        else if (keyLower == "scalex" || keyLower == "scale_x") {
            config.scaleX = std::stof(value);
        }
        else if (keyLower == "scaley" || keyLower == "scale_y") {
            config.scaleY = std::stof(value);
        }
    }
    
    file.close();
    return !config.name.empty() && !config.hatImage.empty();
}

std::string HatConfig::getImagePath(const std::string& imageName) const {
    if (imageName.empty()) return "";
    
    std::filesystem::path folder(folderPath);
    std::filesystem::path image(imageName);
    return (folder / image).string();
}


#include "CatPackConfig.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

bool CatPackConfig::loadFromFile(const std::string& configPath, CatPackConfig& config) {
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
        else if (keyLower == "bodyimage" || keyLower == "body_image") {
            config.bodyImage = value;
        }
        else if (keyLower == "handupimage" || keyLower == "hand_up_image") {
            config.handUpImage = value;
        }
        else if (keyLower == "handdownimage" || keyLower == "hand_down_image") {
            config.handDownImage = value;
        }
        else if (keyLower == "iconimage" || keyLower == "icon_image") {
            config.iconImage = value;
        }
        else if (keyLower == "bodyoffsetx" || keyLower == "body_offset_x") {
            config.bodyOffsetX = std::stof(value);
        }
        else if (keyLower == "bodyoffsety" || keyLower == "body_offset_y") {
            config.bodyOffsetY = std::stof(value);
        }
        else if (keyLower == "bodyoffsetz" || keyLower == "body_offset_z") {
            config.bodyOffsetZ = std::stof(value);
        }
        else if (keyLower == "leftarmoffsetx" || keyLower == "left_arm_offset_x") {
            config.leftArmOffsetX = std::stof(value);
        }
        else if (keyLower == "leftarmoffsety" || keyLower == "left_arm_offset_y") {
            config.leftArmOffsetY = std::stof(value);
        }
        else if (keyLower == "leftarmoffsetz" || keyLower == "left_arm_offset_z") {
            config.leftArmOffsetZ = std::stof(value);
        }
        else if (keyLower == "rightarmoffsetx" || keyLower == "right_arm_offset_x") {
            config.rightArmOffsetX = std::stof(value);
        }
        else if (keyLower == "rightarmoffsety" || keyLower == "right_arm_offset_y") {
            config.rightArmOffsetY = std::stof(value);
        }
        else if (keyLower == "rightarmoffsetz" || keyLower == "right_arm_offset_z") {
            config.rightArmOffsetZ = std::stof(value);
        }
        else if (keyLower == "leftarmspacing" || keyLower == "left_arm_spacing") {
            config.leftArmSpacing = std::stof(value);
        }
        else if (keyLower == "rightarmspacing" || keyLower == "right_arm_spacing") {
            config.rightArmSpacing = std::stof(value);
        }
        else if (keyLower == "punchoffsety" || keyLower == "punch_offset_y") {
            config.punchOffsetY = std::stof(value);
        }
    }
    
    file.close();
    return !config.name.empty() && !config.bodyImage.empty();
}

std::string CatPackConfig::getImagePath(const std::string& imageName) const {
    if (imageName.empty()) return "";
    
    std::filesystem::path folder(folderPath);
    std::filesystem::path image(imageName);
    return (folder / image).string();
}


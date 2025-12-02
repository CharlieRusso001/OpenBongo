#pragma once

#include <string>

struct BonkPackConfig {
    std::string name;                    // Name of the bonk pack
    std::string folderPath;              // Path to the bonk pack folder
    std::string bonkSound;               // Filename for bonk sound effect
    std::string iconImage;               // Filename for icon (optional)
    
    // Volume settings
    float volume = 100.0f;               // Volume (0-100)
    
    // UI ordering weight (higher weight appears earlier in the UI)
    float weight = 0.0f;
    
    // Default constructor
    BonkPackConfig() = default;
    
    // Load configuration from file
    static bool loadFromFile(const std::string& configPath, BonkPackConfig& config);
    
    // Get full path to a sound file
    std::string getSoundPath(const std::string& soundName) const;
    
    // Get full path to an image file
    std::string getImagePath(const std::string& imageName) const;
};


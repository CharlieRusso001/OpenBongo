#pragma once

#include <string>

struct EntitySFXConfig {
    std::string name;                    // Name of the entity SFX
    std::string folderPath;              // Path to the entity SFX folder
    std::string entitySound;             // Filename for entity sound effect
    std::string iconImage;               // Filename for icon (optional)
    
    // Volume settings
    float volume = 100.0f;               // Volume (0-100)
    
    // UI ordering weight (higher weight appears earlier in the UI)
    float weight = 0.0f;
    
    // Default constructor
    EntitySFXConfig() = default;
    
    // Load configuration from file
    static bool loadFromFile(const std::string& configPath, EntitySFXConfig& config);
    
    // Get full path to a sound file
    std::string getSoundPath(const std::string& soundName) const;
    
    // Get full path to an image file
    std::string getImagePath(const std::string& imageName) const;
};


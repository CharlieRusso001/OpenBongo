#pragma once

#include <string>
#include <SFML/Graphics.hpp>

struct HatConfig {
    std::string name;                    // Name of the hat
    std::string folderPath;              // Path to the hat folder
    std::string hatImage;                // Filename for hat image
    std::string iconImage;               // Filename for icon (optional)
    
    // Position offsets (relative to cat body/head position)
    float offsetX = 0.0f;                // X offset from cat head center
    float offsetY = 0.0f;                // Y offset from cat head top
    
    // Size scaling
    float scaleX = 1.0f;                 // Horizontal scale multiplier
    float scaleY = 1.0f;                 // Vertical scale multiplier
    
    // Default constructor
    HatConfig() = default;
    
    // Load configuration from file
    static bool loadFromFile(const std::string& configPath, HatConfig& config);
    
    // Get full path to an image file
    std::string getImagePath(const std::string& imageName) const;
};


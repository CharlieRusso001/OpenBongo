#pragma once

#include <string>
#include <SFML/Graphics.hpp>

struct CatPackConfig {
    std::string name;                    // Name of the cat
    std::string folderPath;              // Path to the cat pack folder
    std::string bodyImage;               // Filename for body image
    std::string handUpImage;             // Filename for hand up image
    std::string handDownImage;           // Filename for hand down image
    std::string iconImage;               // Filename for icon (optional)
    
    // Offsets for positioning parts (relative to body position)
    float bodyOffsetX = 0.0f;
    float bodyOffsetY = 0.0f;
    
    float leftArmOffsetX = 0.0f;
    float leftArmOffsetY = 0.0f;
    
    float rightArmOffsetX = 0.0f;
    float rightArmOffsetY = 0.0f;
    
    // UI ordering weight (higher weight appears earlier in the UI)
    float weight = 0.0f;
    
    // Arm spacing multipliers (for positioning relative to body)
    float leftArmSpacing = 1.1f;         // Multiplier for left arm spacing
    float rightArmSpacing = 1.0f;        // Multiplier for right arm spacing
    
    // Punch animation offset (how much arms move up when punching)
    float punchOffsetY = 0.3f;           // Multiplier for punch animation height
    
    // Hand down texture vertical offset (adjusts position when switching to hand down texture)
    float handDownOffsetY = 0.0f;        // Vertical offset when using hand down texture
    
    // Default constructor
    CatPackConfig() = default;
    
    // Load configuration from file
    static bool loadFromFile(const std::string& configPath, CatPackConfig& config);
    
    // Get full path to an image file
    std::string getImagePath(const std::string& imageName) const;
};


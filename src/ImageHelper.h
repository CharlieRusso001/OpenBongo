#pragma once

#include <string>
#include <SFML/Graphics.hpp>

class ImageHelper {
public:
    // Convert SFML image to base64 PNG string
    static std::string imageToBase64(const sf::Image& image);
    
    // Load image from file and convert to base64
    static std::string imageFileToBase64(const std::string& filePath);
    
    // Convert texture to base64
    static std::string textureToBase64(const sf::Texture& texture);
};


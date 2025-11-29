#include "ImageHelper.h"
#include <sstream>
#include <vector>
#include <fstream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Simple base64 encoding
static const char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const unsigned char* bytes_to_encode, size_t in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

std::string ImageHelper::imageToBase64(const sf::Image& image) {
    sf::Vector2u size = image.getSize();
    if (size.x == 0 || size.y == 0) {
        return "";
    }
    
    // Get pixel data
    const std::uint8_t* pixels = image.getPixelsPtr();
    size_t pixelCount = size.x * size.y * 4; // RGBA
    
    // For simplicity, we'll use a simple PNG encoding approach
    // In a real implementation, you'd want to use a proper PNG library
    // For now, we'll create a minimal PNG structure
    
    // This is a simplified approach - for production, use a proper PNG library
    // like stb_image_write or similar
    std::vector<unsigned char> pngData;
    
    // PNG signature
    pngData.insert(pngData.end(), {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A});
    
    // For a full implementation, you'd need to properly encode PNG chunks
    // This is a placeholder - in production, use a library like stb_image_write
    
    // For now, let's use a simpler approach: save to memory and encode
    // We'll need to use SFML's save functionality or a proper PNG library
    
    // Temporary workaround: save to a temporary file and read it back
    // This is not ideal but works for now
    #ifdef _WIN32
    std::string tempPath = std::filesystem::temp_directory_path().string() + "/temp_bongo_" + std::to_string(GetCurrentProcessId()) + ".png";
    #else
    std::string tempPath = std::filesystem::temp_directory_path().string() + "/temp_bongo_" + std::to_string(getpid()) + ".png";
    #endif
    
    if (image.saveToFile(tempPath)) {
        std::ifstream file(tempPath, std::ios::binary);
        if (file) {
            std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            std::filesystem::remove(tempPath);
            
            return base64_encode(buffer.data(), buffer.size());
        }
    }
    
    return "";
}

std::string ImageHelper::imageFileToBase64(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        return "";
    }
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return "";
    }
    
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    return base64_encode(buffer.data(), buffer.size());
}

std::string ImageHelper::textureToBase64(const sf::Texture& texture) {
    sf::Image image = texture.copyToImage();
    return imageToBase64(image);
}


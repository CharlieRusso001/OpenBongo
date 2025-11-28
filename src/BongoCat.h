#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include "CatPackConfig.h"
#include "HatConfig.h"

class BongoCat {
public:
    BongoCat(float x, float y, float size, const CatPackConfig& config = CatPackConfig());
    
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void punch();
    
    void setPosition(float x, float y);
    sf::Vector2f getPosition() const;
    float getBodyBottomY() const; // Get the bottom Y position of the cat body
    void setWindowHeight(float windowHeight); // Set window height for positioning hands at bottom
    void setConfig(const CatPackConfig& config); // Change cat pack configuration
    void setHat(const HatConfig& hat); // Change hat configuration
    
private:
    sf::Vector2f position;
    float size;
    CatPackConfig config; // Cat pack configuration
    HatConfig hatConfig; // Hat configuration
    
    // Textures for images
    sf::Texture bodyTexture;
    sf::Texture handUpTexture;
    sf::Texture handDownTexture;
    sf::Texture hatTexture;
    
    // Sprites for rendering (using pointers for SFML 3.0 compatibility)
    std::unique_ptr<sf::Sprite> bodySprite;
    std::unique_ptr<sf::Sprite> leftArmSprite;
    std::unique_ptr<sf::Sprite> rightArmSprite;
    std::unique_ptr<sf::Sprite> hatSprite;
    
    // Animation state
    bool isPunching;
    float punchTimer;
    float punchDuration;
    
    // Arm alternation state
    bool leftArmActive; // true if left arm is currently active, false for right arm
    int punchCount; // Count punches to alternate arms
    
    // Arm positions
    sf::Vector2f leftArmRestPos;
    sf::Vector2f rightArmRestPos;
    sf::Vector2f leftArmPunchPos;
    sf::Vector2f rightArmPunchPos;
    
    // Arm sizes (from texture dimensions)
    float armWidth;
    float armHeight;
    
    // Body display dimensions (accounting for aspect ratio)
    float bodyDisplayHeight;
    
    // Window height for positioning hands at bottom
    float windowHeight;
    
    // Cached scale values for arm sprites
    sf::Vector2f leftArmScale;
    sf::Vector2f rightArmScale;
    sf::Vector2f rightArmOrigin;
    
    // Track current texture state to avoid unnecessary sprite recreation
    bool usingHandDownTexture;
    
    void updateAnimation(float deltaTime);
    void updateArmPositions();
    bool loadTextures();
    bool loadHatTexture(); // Load hat texture
    void recalculatePositions(); // Recalculate positions when config changes
    void updateHatPosition(); // Update hat position relative to body
};


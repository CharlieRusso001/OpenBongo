#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include "config/CatPackConfig.h"
#include "config/HatConfig.h"

class BongoCat {
public:
    BongoCat(float x, float y, float size, const CatPackConfig& config = CatPackConfig());
    
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void punch();
    
    void setPosition(float x, float y);
    sf::Vector2f getPosition() const;
    float getBodyBottomY() const; // Get the bottom Y position of the cat body
    float getBodyDisplayHeight() const; // Get the display height of the cat body
    void setWindowHeight(float windowHeight); // Set window height for positioning hands at bottom
    void setConfig(const CatPackConfig& config); // Change cat pack configuration
    void setHat(const HatConfig& hat); // Change hat configuration
    void setSize(float newSize); // Change cat size
    void setFlip(bool flipped); // Flip cat horizontally (mirror on vertical line)
    void setLeftArmOffset(float offsetX); // Adjust left arm horizontal offset (negative = left, positive = right)
    void setRightArmOffset(float offsetX); // Adjust right arm horizontal offset (negative = left, positive = right) - 0 means base offset for size
    void setAnimationVerticalOffset(float offsetY); // Adjust how much arms move down when typing - 0 means base offset for size
    
private:
    // Get base offsets for current size (these are applied when slider is at 0)
    float getBaseRightArmOffset() const;
    float getBaseAnimationVerticalOffset() const;
    
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
    
    // Flip state
    bool isFlipped;
    
    // User-adjustable arm offsets
    float leftArmOffsetX;
    float rightArmOffsetX;
    float animationVerticalOffset;
    
    void updateAnimation(float deltaTime);
    void updateArmPositions();
    bool loadTextures();
    bool loadHatTexture(); // Load hat texture
    void recalculatePositions(); // Recalculate positions when config changes
    void updateHatPosition(); // Update hat position relative to body
    void applyFlip(); // Apply flip state to sprites
};


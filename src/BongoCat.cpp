#include "BongoCat.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <random>

#ifdef _WIN32
#include <windows.h>
#endif

bool BongoCat::loadHatTexture() {
    if (hatConfig.hatImage.empty() || hatConfig.folderPath.empty()) {
        hatSprite.reset();
        return false;
    }
    
    std::string hatPath = hatConfig.getImagePath(hatConfig.hatImage);
    
    if (!hatTexture.loadFromFile(hatPath)) {
        hatSprite.reset();
        return false;
    }
    
    // Create sprite and set up scaling
    hatSprite = std::make_unique<sf::Sprite>(hatTexture);
    
    // Get texture dimensions and apply scale
    sf::Vector2u hatTexSize = hatTexture.getSize();
    if (hatTexSize.x > 0 && hatTexSize.y > 0) {
        // Scale based on cat size and hat config scale
        float baseScaleX = (size * 0.8f) / static_cast<float>(hatTexSize.x); // Hat is slightly smaller than body
        float baseScaleY = (size * 0.8f) / static_cast<float>(hatTexSize.y);
        
        // Apply hat config scale multipliers
        float finalScaleX = baseScaleX * hatConfig.scaleX;
        float finalScaleY = baseScaleY * hatConfig.scaleY;
        
        hatSprite->setScale(sf::Vector2f(finalScaleX, finalScaleY));
    }
    
    updateHatPosition();
    return true;
}

void BongoCat::updateHatPosition() {
    if (!hatSprite || !bodySprite) {
        return;
    }
    
    // Position hat relative to top of body (head area)
    sf::Vector2f bodyPos = bodySprite->getPosition(); // This is now the bottom center position
    sf::Vector2u bodyTexSize = bodyTexture.getSize();
    
    // Calculate body dimensions
    float bodyDisplayWidth = size;
    float bodyDisplayHeight = size;
    if (bodyTexSize.x > 0 && bodyTexSize.y > 0) {
        float aspectRatio = static_cast<float>(bodyTexSize.y) / static_cast<float>(bodyTexSize.x);
        bodyDisplayHeight = size * aspectRatio;
    }
    
    // Calculate body top (head position) from bottom center
    float bodyTopY = bodyPos.y - bodyDisplayHeight;
    float bodyLeftX = bodyPos.x - bodyDisplayWidth / 2.0f;
    
    // Position hat at top of body with offsets
    float hatX = bodyPos.x + hatConfig.offsetX; // Center on body (bodyPos.x is already center), then apply offset
    float hatY = bodyTopY + hatConfig.offsetY;
    
    // Adjust for hat sprite center if needed (hats typically center horizontally)
    sf::Vector2u hatTexSize = hatTexture.getSize();
    if (hatTexSize.x > 0) {
        float hatWidth = (size * 0.8f) * hatConfig.scaleX;
        hatX -= hatWidth / 2.0f; // Center the hat horizontally
    }
    
    hatSprite->setPosition(sf::Vector2f(hatX, hatY));
}

bool BongoCat::loadTextures() {
    bool loaded = false;
    
    if (!config.folderPath.empty() && !config.bodyImage.empty()) {
        // Use config paths
        std::string bodyPath = config.getImagePath(config.bodyImage);
        std::string handUpPath = config.getImagePath(config.handUpImage);
        std::string handDownPath = config.getImagePath(config.handDownImage);
        
        // Write debug info to log file
        std::ofstream logFile("OpenBongo.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "[BongoCat] Loading from config: " << config.name << std::endl;
            logFile << "  Body: " << bodyPath << std::endl;
            logFile << "  HandUp: " << handUpPath << std::endl;
            logFile << "  HandDown: " << handDownPath << std::endl;
            logFile.close();
        }
        
        if (bodyTexture.loadFromFile(bodyPath) &&
            handUpTexture.loadFromFile(handUpPath) &&
            handDownTexture.loadFromFile(handDownPath)) {
            loaded = true;
        }
    }
    
    // Fallback to default DevArt paths if config not available or failed
    if (!loaded) {
        std::vector<std::string> basePaths;
        basePaths.push_back("catpacks/DevArt/");
        basePaths.push_back("catpacks\\DevArt\\");
        basePaths.push_back("../catpacks/DevArt/");
        basePaths.push_back("..\\catpacks\\DevArt\\");
        basePaths.push_back("../../catpacks/DevArt/");
        basePaths.push_back("..\\..\\catpacks\\DevArt\\");
        
        #ifdef _WIN32
        char exePath[MAX_PATH];
        if (GetModuleFileNameA(nullptr, exePath, MAX_PATH) != 0) {
            std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
            basePaths.push_back((exeDir / "catpacks" / "DevArt").string() + "\\");
            basePaths.push_back((exeDir / "catpacks" / "DevArt").string() + "/");
            basePaths.push_back((exeDir.parent_path() / "catpacks" / "DevArt").string() + "\\");
            basePaths.push_back((exeDir.parent_path() / "catpacks" / "DevArt").string() + "/");
        }
        #endif
        
        std::ofstream logFile("OpenBongo.log", std::ios::app);
        for (const std::string& basePath : basePaths) {
            std::string bodyPath = basePath + "body-devartcat.png";
            std::string handUpPath = basePath + "handup-devartcat.png";
            std::string handDownPath = basePath + "handdown-devartcat.png";
            
            if (bodyTexture.loadFromFile(bodyPath) &&
                handUpTexture.loadFromFile(handUpPath) &&
                handDownTexture.loadFromFile(handDownPath)) {
                if (logFile.is_open()) {
                    logFile << "[BongoCat] Successfully loaded default cat images from: " << basePath << std::endl;
                    logFile.close();
                }
                loaded = true;
                break;
            }
        }
        
        if (!loaded && logFile.is_open()) {
            logFile << "[BongoCat] ERROR: Failed to load textures" << std::endl;
            logFile.close();
        }
    }
    
    if (!loaded) {
        return false;
    }
    
    // Set up body sprite
    bodySprite = std::make_unique<sf::Sprite>(bodyTexture);
    
    // Get texture dimensions and scale to match desired size
    sf::Vector2u bodyTexSize = bodyTexture.getSize();
    if (bodyTexSize.x > 0 && bodyTexSize.y > 0) {
        // Set origin to bottom center so sprite scales from bottom center
        bodySprite->setOrigin(sf::Vector2f(static_cast<float>(bodyTexSize.x) / 2.0f, static_cast<float>(bodyTexSize.y)));
        
        float scaleX = size / static_cast<float>(bodyTexSize.x);
        float scaleY = size / static_cast<float>(bodyTexSize.y);
        bodySprite->setScale(sf::Vector2f(scaleX, scaleY));
    }
    // Position will be set correctly in recalculatePositions() which is called after loadTextures()
    // Set a temporary position here to avoid issues
    bodySprite->setPosition(sf::Vector2f(position.x + config.bodyOffsetX, position.y + config.bodyOffsetY));
    
    // Set up arm sprites
    sf::Vector2u handTexSize = handUpTexture.getSize();
    if (handTexSize.x > 0 && handTexSize.y > 0) {
        // Use a proportion of the body size for arms (1.5x larger)
        armWidth = size * 0.3f * 1.5f;
        armHeight = size * 0.4f * 1.5f;
        
        float armScaleX = armWidth / static_cast<float>(handTexSize.x);
        float armScaleY = armHeight / static_cast<float>(handTexSize.y);
        
        // Cache scale values (same for both arms - no mirroring)
        leftArmScale = sf::Vector2f(armScaleX, armScaleY);
        rightArmScale = sf::Vector2f(armScaleX, armScaleY);
        rightArmOrigin = sf::Vector2f(0.0f, 0.0f); // No origin offset needed since we're not flipping
        
        leftArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
        rightArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
        
        leftArmSprite->setScale(leftArmScale);
        rightArmSprite->setScale(rightArmScale);
    } else {
        // Create sprites with hand up texture even if size is 0
        leftArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
        rightArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
    }
    
    return true;
}

BongoCat::BongoCat(float x, float y, float size, const CatPackConfig& config)
    : position(x, y), size(size), config(config), hatConfig(), isPunching(false), punchTimer(0.0f), punchDuration(0.15f),
      armWidth(size * 0.3f * 1.5f), armHeight(size * 0.4f * 1.5f), usingHandDownTexture(false), bodyDisplayHeight(size), windowHeight(200.0f),
      leftArmActive(true), punchCount(0) {
    
    // Load textures
    bool texturesLoaded = loadTextures();
    
    if (!texturesLoaded) {
        std::cerr << "Warning: Failed to load cat images. The cat may not display correctly." << std::endl;
    }
    
    recalculatePositions();
    updateArmPositions();
    loadHatTexture(); // Load hat (will be empty/no hat by default)
}

void BongoCat::setConfig(const CatPackConfig& newConfig) {
    config = newConfig;
    bool texturesLoaded = loadTextures();
    if (texturesLoaded) {
        recalculatePositions();
        updateArmPositions();
        updateHatPosition(); // Update hat position when body changes
    }
}

void BongoCat::setHat(const HatConfig& hat) {
    hatConfig = hat;
    loadHatTexture();
}

void BongoCat::setSize(float newSize) {
    if (newSize <= 0.0f) return; // Invalid size
    
    size = newSize;
    
    // Recalculate arm sizes based on new body size
    armWidth = size * 0.3f * 1.5f;
    armHeight = size * 0.4f * 1.5f;
    
    // Reload textures to update scales
    bool texturesLoaded = loadTextures();
    if (texturesLoaded) {
        recalculatePositions();
        updateArmPositions();
        updateHatPosition();
    }
}

void BongoCat::recalculatePositions() {
    // Calculate arm positions based on body position, size, and config offsets
    sf::Vector2u bodyTexSize = bodyTexture.getSize();
    float bodyDisplayWidth = size;
    float bodyDisplayHeight = size;
    
    if (bodyTexSize.x > 0 && bodyTexSize.y > 0) {
        // Maintain aspect ratio
        float aspectRatio = static_cast<float>(bodyTexSize.y) / static_cast<float>(bodyTexSize.x);
        bodyDisplayHeight = size * aspectRatio;
    } else {
        bodyDisplayHeight = size;
    }
    
    // Apply body offset
    float bodyX = position.x + config.bodyOffsetX;
    float bodyY = position.y + config.bodyOffsetY;
    
    // Position arms so bottom of hand images aligns with bottom of body image
    float bodyBottomY = bodyY + bodyDisplayHeight;
    float handY = bodyBottomY - armHeight; // Position hands so their bottom aligns with body bottom
    
    // Apply config offsets and spacing multipliers
    float leftArmX = bodyX - armWidth * 0.3f * config.leftArmSpacing + config.leftArmOffsetX;
    float rightArmX = bodyX + bodyDisplayWidth - armWidth * 0.35f * config.rightArmSpacing + config.rightArmOffsetX;
    
    leftArmRestPos = sf::Vector2f(leftArmX, handY + config.leftArmOffsetY);
    rightArmRestPos = sf::Vector2f(rightArmX, handY + config.rightArmOffsetY);
    
    // Punch positions (move up by punch offset multiplier)
    leftArmPunchPos = sf::Vector2f(leftArmX, handY + config.leftArmOffsetY - armHeight * config.punchOffsetY);
    rightArmPunchPos = sf::Vector2f(rightArmX, handY + config.rightArmOffsetY - armHeight * config.punchOffsetY);
    
    // Update body sprite position with offset
    // Since origin is at bottom center, position should be at bottom center of desired location
    if (bodySprite) {
        float bodyCenterX = bodyX + bodyDisplayWidth / 2.0f;
        float bodyBottomCenterY = bodyBottomY;
        bodySprite->setPosition(sf::Vector2f(bodyCenterX, bodyBottomCenterY));
    }
    
    // Update hat position when body position changes
    updateHatPosition();
}

void BongoCat::update(float deltaTime) {
    updateAnimation(deltaTime);
}

void BongoCat::draw(sf::RenderWindow& window) {
    if (bodySprite) {
        window.draw(*bodySprite);
    }
    if (leftArmSprite) {
        window.draw(*leftArmSprite);
    }
    if (rightArmSprite) {
        window.draw(*rightArmSprite);
    }
    // Draw hat last so it appears on top of everything
    if (hatSprite) {
        window.draw(*hatSprite);
    }
}

void BongoCat::punch() {
    isPunching = true;
    punchTimer = 0.0f;
    
    // Alternate arms with some randomization
    punchCount++;
    
    // Randomly decide to alternate or keep same arm (70% chance to alternate, 30% to keep same)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    float randVal = dis(gen);
    if (randVal < 0.7f || punchCount == 1) {
        // Alternate to the other arm
        leftArmActive = !leftArmActive;
    }
    // Otherwise keep the same arm active
}

void BongoCat::setPosition(float x, float y) {
    position = sf::Vector2f(x, y);
    recalculatePositions();
    updateArmPositions();
}

sf::Vector2f BongoCat::getPosition() const {
    return position;
}

float BongoCat::getBodyBottomY() const {
    return position.y + bodyDisplayHeight;
}

void BongoCat::setWindowHeight(float windowHeight) {
    this->windowHeight = windowHeight;
    // Recalculate arm positions when window height changes
    recalculatePositions();
    updateArmPositions();
}

void BongoCat::updateAnimation(float deltaTime) {
    if (isPunching) {
        punchTimer += deltaTime;
        
        if (punchTimer >= punchDuration) {
            isPunching = false;
            punchTimer = 0.0f;
        }
    }
    
    updateArmPositions();
}

void BongoCat::updateArmPositions() {
    float leftProgress = 0.0f;
    float rightProgress = 0.0f;
    
    if (isPunching) {
        // Ease in-out animation
        float t = punchTimer / punchDuration;
        float animProgress = t < 0.5f 
            ? 2.0f * t * t 
            : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        
        // Only animate the active arm
        if (leftArmActive) {
            leftProgress = animProgress;
            rightProgress = 0.0f; // Right arm stays at rest
        } else {
            leftProgress = 0.0f; // Left arm stays at rest
            rightProgress = animProgress;
        }
        
        // Use hand down texture for the active arm when punching
        if (leftArmSprite && rightArmSprite) {
            if (leftArmActive) {
                // Left arm is active - update left arm texture to hand down
                leftArmSprite = std::make_unique<sf::Sprite>(handDownTexture);
                leftArmSprite->setScale(leftArmScale);
                // Right arm stays with hand up texture
                rightArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
                rightArmSprite->setScale(rightArmScale);
            } else {
                // Right arm is active - update right arm texture to hand down
                rightArmSprite = std::make_unique<sf::Sprite>(handDownTexture);
                rightArmSprite->setScale(rightArmScale);
                // Left arm stays with hand up texture
                leftArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
                leftArmSprite->setScale(leftArmScale);
            }
            usingHandDownTexture = true;
        }
    } else {
        // Use hand up texture for both arms when idle
        if (leftArmSprite && rightArmSprite && usingHandDownTexture) {
            leftArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
            rightArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
            leftArmSprite->setScale(leftArmScale);
            rightArmSprite->setScale(rightArmScale);
            usingHandDownTexture = false;
        }
    }
    
    // Interpolate between rest and punch positions (only active arm moves)
    sf::Vector2f leftPos = sf::Vector2f(
        leftArmRestPos.x + (leftArmPunchPos.x - leftArmRestPos.x) * leftProgress,
        leftArmRestPos.y + (leftArmPunchPos.y - leftArmRestPos.y) * leftProgress
    );
    
    sf::Vector2f rightPos = sf::Vector2f(
        rightArmRestPos.x + (rightArmPunchPos.x - rightArmRestPos.x) * rightProgress,
        rightArmRestPos.y + (rightArmPunchPos.y - rightArmRestPos.y) * rightProgress
    );
    
    if (leftArmSprite) {
        leftArmSprite->setPosition(leftPos);
    }
    if (rightArmSprite) {
        rightArmSprite->setPosition(rightPos);
    }
}


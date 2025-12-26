#include "core/BongoCat.h"
#include "utils/Logger.h"
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
        
        // Write debug info to log
        LOG_INFO("[BongoCat] Loading from config: " + config.name);
        LOG_INFO("  Body: " + bodyPath);
        LOG_INFO("  HandUp: " + handUpPath);
        LOG_INFO("  HandDown: " + handDownPath);
        
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
        
        for (const std::string& basePath : basePaths) {
            std::string bodyPath = basePath + "body-devartcat.png";
            std::string handUpPath = basePath + "handup-devartcat.png";
            std::string handDownPath = basePath + "handdown-devartcat.png";
            
            if (bodyTexture.loadFromFile(bodyPath) &&
                handUpTexture.loadFromFile(handUpPath) &&
                handDownTexture.loadFromFile(handDownPath)) {
                LOG_INFO("[BongoCat] Successfully loaded default cat images from: " + basePath);
                loaded = true;
                break;
            }
        }
        
        if (!loaded) {
            LOG_ERROR("[BongoCat] ERROR: Failed to load textures");
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
        
        // Set origin to center horizontally and bottom vertically so arms flip around their center
        leftArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handTexSize.x) / 2.0f, static_cast<float>(handTexSize.y)));
        rightArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handTexSize.x) / 2.0f, static_cast<float>(handTexSize.y)));
        
        leftArmSprite->setScale(leftArmScale);
        rightArmSprite->setScale(rightArmScale);
    } else {
        // Create sprites with hand up texture even if size is 0
        leftArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
        rightArmSprite = std::make_unique<sf::Sprite>(handUpTexture);
        if (handTexSize.x > 0 && handTexSize.y > 0) {
            leftArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handTexSize.x) / 2.0f, static_cast<float>(handTexSize.y)));
            rightArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handTexSize.x) / 2.0f, static_cast<float>(handTexSize.y)));
        }
    }
    
    return true;
}

BongoCat::BongoCat(float x, float y, float size, const CatPackConfig& config)
    : position(x, y), size(size), config(config), hatConfig(), isPunching(false), punchTimer(0.0f), punchDuration(0.15f),
      armWidth(size * 0.3f * 1.5f), armHeight(size * 0.4f * 1.5f), usingHandDownTexture(false), bodyDisplayHeight(size), windowHeight(200.0f),
      leftArmActive(true), punchCount(0), isFlipped(false), leftArmOffsetX(0.0f), rightArmOffsetX(0.0f), animationVerticalOffset(0.0f) {
    
    // Load textures
    bool texturesLoaded = loadTextures();
    
    if (!texturesLoaded) {
        std::cerr << "Warning: Failed to load cat images. The cat may not display correctly." << std::endl;
    }
    
    recalculatePositions();
    updateArmPositions();
    loadHatTexture(); // Load hat (will be empty/no hat by default)
    applyFlip(); // Apply initial flip state
}

void BongoCat::setConfig(const CatPackConfig& newConfig) {
    config = newConfig;
    bool texturesLoaded = loadTextures();
    if (texturesLoaded) {
        recalculatePositions();
        updateArmPositions();
        updateHatPosition(); // Update hat position when body changes
        applyFlip(); // Reapply flip after reloading textures
    }
}

void BongoCat::setHat(const HatConfig& hat) {
    hatConfig = hat;
    loadHatTexture();
    applyFlip(); // Reapply flip after loading hat
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
        applyFlip(); // Reapply flip after reloading textures
    }
}

void BongoCat::setFlip(bool flipped) {
    isFlipped = flipped;
    recalculatePositions(); // Recalculate positions to swap left/right when flipped
    applyFlip();
}

float BongoCat::getBaseRightArmOffset() const {
    // Base offset for all sizes (applied when slider is at 0)
    // Negative value moves arm inward (closer to body)
    return -30.0f;
}

float BongoCat::getBaseAnimationVerticalOffset() const {
    // Base offsets for different sizes (applied when slider is at 0)
    if (size >= 140.0f) {
        // Big (150)
        return -55.0f;
    } else {
        // Default (100) and Small (73)
        return -15.0f;
    }
}

void BongoCat::setLeftArmOffset(float offsetX) {
    leftArmOffsetX = offsetX;
    recalculatePositions();
    updateArmPositions();
}

void BongoCat::setRightArmOffset(float offsetX) {
    // offsetX is the slider value (0 = base offset, user can adjust from there)
    // Add base offset for current size
    rightArmOffsetX = offsetX + getBaseRightArmOffset();
    recalculatePositions();
    updateArmPositions();
}

void BongoCat::setAnimationVerticalOffset(float offsetY) {
    // offsetY is the slider value (0 = base offset, user can adjust from there)
    // Add base offset for current size
    animationVerticalOffset = offsetY + getBaseAnimationVerticalOffset();
    recalculatePositions();
    updateArmPositions();
}

void BongoCat::applyFlip() {
    if (!bodySprite) return;
    
    // Get current scale
    sf::Vector2f currentScale = bodySprite->getScale();
    
    // Apply horizontal flip by negating X scale
    if (isFlipped) {
        bodySprite->setScale(sf::Vector2f(-std::abs(currentScale.x), currentScale.y));
    } else {
        bodySprite->setScale(sf::Vector2f(std::abs(currentScale.x), currentScale.y));
    }
    
    // Also flip arms
    if (leftArmSprite) {
        sf::Vector2f leftScale = leftArmSprite->getScale();
        if (isFlipped) {
            leftArmSprite->setScale(sf::Vector2f(-std::abs(leftScale.x), leftScale.y));
        } else {
            leftArmSprite->setScale(sf::Vector2f(std::abs(leftScale.x), leftScale.y));
        }
    }
    
    if (rightArmSprite) {
        sf::Vector2f rightScale = rightArmSprite->getScale();
        if (isFlipped) {
            rightArmSprite->setScale(sf::Vector2f(-std::abs(rightScale.x), rightScale.y));
        } else {
            rightArmSprite->setScale(sf::Vector2f(std::abs(rightScale.x), rightScale.y));
        }
    }
    
    // Also flip hat
    if (hatSprite) {
        sf::Vector2f hatScale = hatSprite->getScale();
        if (isFlipped) {
            hatSprite->setScale(sf::Vector2f(-std::abs(hatScale.x), hatScale.y));
        } else {
            hatSprite->setScale(sf::Vector2f(std::abs(hatScale.x), hatScale.y));
        }
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
    float handY = bodyBottomY; // Position hands at body bottom (origin is at bottom center of arm sprite)
    
    // Calculate body center for mirroring when flipped
    float bodyCenterX = bodyX + bodyDisplayWidth / 2.0f;
    
    // Calculate original arm positions with size-dependent spacing
    // Left arm: use square root scaling to maintain consistent proportions
    // Right arm: use consistent scaling for all sizes to keep it closer to body
    float sizeScale = size / 100.0f; // 0.73 for small, 1.0 for default, 1.5 for big
    float leftSpacingScale = std::sqrt(sizeScale); // Square root for left arm (consistent proportions)
    // For right arm: use smaller multiplier to keep it closer to body for all sizes
    float rightSpacingScale = sizeScale * 0.8f; // Reduced scaling to move arm closer to body
    
    float leftArmXOriginal = bodyX - bodyDisplayWidth * 0.15f * leftSpacingScale * config.leftArmSpacing + config.leftArmOffsetX;
    // For right arm: reduced spacing to keep it closer to body
    float rightArmXOriginal = bodyX + bodyDisplayWidth - bodyDisplayWidth * 0.15f * (1.0f - rightSpacingScale) * config.rightArmSpacing + config.rightArmOffsetX;
    
    // Adjust for center origin: add half width to left, subtract half width from right
    // This positions the center of the sprite where the left edge was originally
    leftArmXOriginal += armWidth / 2.0f;
    rightArmXOriginal -= armWidth / 2.0f;
    
    // When flipped, mirror the positions around the body center
    float leftArmX, rightArmX;
    if (isFlipped) {
        // Mirror positions: distance from center becomes negative distance from center
        // This swaps left and right while maintaining the same spacing
        leftArmX = 2.0f * bodyCenterX - rightArmXOriginal;
        rightArmX = 2.0f * bodyCenterX - leftArmXOriginal;
    } else {
        leftArmX = leftArmXOriginal;
        rightArmX = rightArmXOriginal;
    }
    
    // Apply user-adjustable horizontal offsets
    leftArmX += leftArmOffsetX;
    rightArmX += rightArmOffsetX;
    
    leftArmRestPos = sf::Vector2f(leftArmX, handY + config.leftArmOffsetY);
    rightArmRestPos = sf::Vector2f(rightArmX, handY + config.rightArmOffsetY);
    
    // Punch positions (move down further when typing - increased distance + user offset)
    float punchDistance = armHeight * config.punchOffsetY * 1.8f + animationVerticalOffset;
    leftArmPunchPos = sf::Vector2f(leftArmX, handY + config.leftArmOffsetY - punchDistance);
    rightArmPunchPos = sf::Vector2f(rightArmX, handY + config.rightArmOffsetY - punchDistance);
    
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

float BongoCat::getBodyDisplayHeight() const {
    return bodyDisplayHeight;
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
            sf::Vector2u handUpTexSize = handUpTexture.getSize();
            sf::Vector2u handDownTexSize = handDownTexture.getSize();
            
            // Only update if we weren't already using hand down texture or if we switched active arms
            // Note: SFML 3.0 getTexture() returns a reference, so take address for comparison
            if (!usingHandDownTexture || 
                (leftArmActive && &leftArmSprite->getTexture() != &handDownTexture) ||
                (!leftArmActive && &rightArmSprite->getTexture() != &handDownTexture)) {
                
                if (leftArmActive) {
                    // Left arm is active - update left arm texture to hand down
                    leftArmSprite->setTexture(handDownTexture, true);
                    if (handDownTexSize.x > 0 && handDownTexSize.y > 0) {
                        leftArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handDownTexSize.x) / 2.0f, static_cast<float>(handDownTexSize.y)));
                    }
                    leftArmSprite->setScale(leftArmScale);
                    
                    // Right arm stays with hand up texture
                    rightArmSprite->setTexture(handUpTexture, true);
                    if (handUpTexSize.x > 0 && handUpTexSize.y > 0) {
                        rightArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handUpTexSize.x) / 2.0f, static_cast<float>(handUpTexSize.y)));
                    }
                    rightArmSprite->setScale(rightArmScale);
                } else {
                    // Right arm is active - update right arm texture to hand down
                    rightArmSprite->setTexture(handDownTexture, true);
                    if (handDownTexSize.x > 0 && handDownTexSize.y > 0) {
                        rightArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handDownTexSize.x) / 2.0f, static_cast<float>(handDownTexSize.y)));
                    }
                    rightArmSprite->setScale(rightArmScale);
                    
                    // Left arm stays with hand up texture
                    leftArmSprite->setTexture(handUpTexture, true);
                    if (handUpTexSize.x > 0 && handUpTexSize.y > 0) {
                        leftArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handUpTexSize.x) / 2.0f, static_cast<float>(handUpTexSize.y)));
                    }
                    leftArmSprite->setScale(leftArmScale);
                }
                usingHandDownTexture = true;
                // Reapply flip after updating textures
                applyFlip();
            }
        }
    } else {
        // Use hand up texture for both arms when idle
        if (leftArmSprite && rightArmSprite && usingHandDownTexture) {
            sf::Vector2u handTexSize = handUpTexture.getSize();
            
            leftArmSprite->setTexture(handUpTexture, true);
            rightArmSprite->setTexture(handUpTexture, true);
            
            if (handTexSize.x > 0 && handTexSize.y > 0) {
                leftArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handTexSize.x) / 2.0f, static_cast<float>(handTexSize.y)));
                rightArmSprite->setOrigin(sf::Vector2f(static_cast<float>(handTexSize.x) / 2.0f, static_cast<float>(handTexSize.y)));
            }
            
            leftArmSprite->setScale(leftArmScale);
            rightArmSprite->setScale(rightArmScale);
            
            usingHandDownTexture = false;
            // Reapply flip after updating textures
            applyFlip();
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
    
    // Apply hand down offset when using hand down texture
    if (usingHandDownTexture) {
        if (leftArmActive) {
            leftPos.y += config.handDownOffsetY;
        } else {
            rightPos.y += config.handDownOffsetY;
        }
    }
    
    if (leftArmSprite) {
        leftArmSprite->setPosition(leftPos);
    }
    if (rightArmSprite) {
        rightArmSprite->setPosition(rightPos);
    }
    
    // Always re-apply flip state because setScale above might have reset it (if we used setScale with positive values)
    applyFlip();
}


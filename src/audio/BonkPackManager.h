#pragma once

#include "audio/BonkPackConfig.h"
#include <vector>
#include <string>

class BonkPackManager {
public:
    // Scan for all available bonk packs
    static std::vector<BonkPackConfig> scanForBonkPacks(const std::string& basePath = "sfx/bonkpacks");
    
    // Get default bonk pack (silent fallback)
    static BonkPackConfig getDefaultBonkPack();
    
    // Find bonk pack by name
    static BonkPackConfig findBonkPackByName(const std::vector<BonkPackConfig>& packs, const std::string& name);
};


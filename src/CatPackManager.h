#pragma once

#include "CatPackConfig.h"
#include <vector>
#include <string>

class CatPackManager {
public:
    // Scan for all available cat packs
    static std::vector<CatPackConfig> scanForCatPacks(const std::string& basePath = "catpacks");
    
    // Get default cat pack (DevArt fallback)
    static CatPackConfig getDefaultCatPack();
    
    // Find cat pack by name
    static CatPackConfig findCatPackByName(const std::vector<CatPackConfig>& packs, const std::string& name);
};


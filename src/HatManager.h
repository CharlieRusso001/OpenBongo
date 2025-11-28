#pragma once

#include "HatConfig.h"
#include <vector>
#include <string>

class HatManager {
public:
    // Scan for all available hats
    static std::vector<HatConfig> scanForHats(const std::string& basePath = "hats");
    
    // Get default "no hat" config
    static HatConfig getNoHat();
    
    // Find hat by name
    static HatConfig findHatByName(const std::vector<HatConfig>& hats, const std::string& name);
};


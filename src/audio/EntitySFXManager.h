#pragma once

#include "audio/EntitySFXConfig.h"
#include <vector>
#include <string>

class EntitySFXManager {
public:
    // Scan for all available entity SFX
    static std::vector<EntitySFXConfig> scanForEntitySFX(const std::string& basePath = "sfx/entitysfx");
    
    // Get default entity SFX (silent fallback)
    static EntitySFXConfig getDefaultEntitySFX();
    
    // Find entity SFX by name
    static EntitySFXConfig findEntitySFXByName(const std::vector<EntitySFXConfig>& sfx, const std::string& name);
};


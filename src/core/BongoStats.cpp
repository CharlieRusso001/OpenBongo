#include "core/BongoStats.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <vector>
#include <mutex>

#ifdef _WIN32
#include <windows.h>
#endif

void BongoStats::initialize(const std::string& statsFilePath) {
    std::lock_guard<std::mutex> lock(statsMutex);
    this->statsFilePath = statsFilePath;
    loadStats();
}

void BongoStats::recordKeyPress(unsigned int keyCode) {
    std::lock_guard<std::mutex> lock(statsMutex);
    keyPressCounts[keyCode]++;
}

void BongoStats::recordMouseClick(const std::string& buttonName) {
    std::lock_guard<std::mutex> lock(statsMutex);
    mouseButtonCounts[buttonName]++;
}

int BongoStats::getKeyCount(unsigned int keyCode) const {
    std::lock_guard<std::mutex> lock(statsMutex);
    auto it = keyPressCounts.find(keyCode);
    return (it != keyPressCounts.end()) ? it->second : 0;
}

int BongoStats::getMouseButtonCount(const std::string& buttonName) const {
    std::lock_guard<std::mutex> lock(statsMutex);
    auto it = mouseButtonCounts.find(buttonName);
    return (it != mouseButtonCounts.end()) ? it->second : 0;
}

std::string BongoStats::getKeyName(unsigned int keyCode) const {
#ifdef _WIN32
    // Windows virtual key codes
    switch (keyCode) {
        case VK_SPACE: return "SPACE";
        case VK_RETURN: return "ENTER";
        case VK_TAB: return "TAB";
        case VK_ESCAPE: return "ESC";
        case VK_BACK: return "BACKSPACE";
        case VK_DELETE: return "DELETE";
        case VK_INSERT: return "INSERT";
        case VK_HOME: return "HOME";
        case VK_END: return "END";
        case VK_PRIOR: return "PAGE_UP";
        case VK_NEXT: return "PAGE_DOWN";
        case VK_LEFT: return "LEFT_ARROW";
        case VK_RIGHT: return "RIGHT_ARROW";
        case VK_UP: return "UP_ARROW";
        case VK_DOWN: return "DOWN_ARROW";
        case VK_SHIFT: return "SHIFT";
        case VK_CONTROL: return "CTRL";
        case VK_MENU: return "ALT";
        case VK_LWIN: return "LEFT_WIN";
        case VK_RWIN: return "RIGHT_WIN";
        case VK_CAPITAL: return "CAPS_LOCK";
        case VK_NUMLOCK: return "NUM_LOCK";
        case VK_SCROLL: return "SCROLL_LOCK";
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        default:
            // Check if it's a letter (A-Z)
            if (keyCode >= 'A' && keyCode <= 'Z') {
                return std::string(1, static_cast<char>(keyCode));
            }
            // Check if it's a number (0-9)
            if (keyCode >= '0' && keyCode <= '9') {
                return std::string(1, static_cast<char>(keyCode));
            }
            // Check if it's a numpad key
            if (keyCode >= VK_NUMPAD0 && keyCode <= VK_NUMPAD9) {
                return "NUMPAD" + std::string(1, static_cast<char>('0' + (keyCode - VK_NUMPAD0)));
            }
            return "KEY_" + std::to_string(keyCode);
    }
#else
    // For non-Windows, just return the code as string
    return "KEY_" + std::to_string(keyCode);
#endif
}

std::string BongoStats::formatStats() const {
    std::ostringstream oss;
    
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    oss << "=== Bongo Stats - Last Updated: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " ===" << std::endl;
    oss << std::endl;
    
    // Mouse button stats
    oss << "--- Mouse Button Clicks ---" << std::endl;
    int totalMouseClicks = 0;
    for (const auto& pair : mouseButtonCounts) {
        oss << pair.first << ": " << pair.second << std::endl;
        totalMouseClicks += pair.second;
    }
    oss << "TOTAL MOUSE CLICKS: " << totalMouseClicks << std::endl;
    oss << std::endl;
    
    // Keyboard stats
    oss << "--- Keyboard Key Presses ---" << std::endl;
    
    // Sort by count (descending) for better readability
    std::vector<std::pair<unsigned int, int>> sortedKeys;
    for (const auto& pair : keyPressCounts) {
        sortedKeys.push_back(pair);
    }
    std::sort(sortedKeys.begin(), sortedKeys.end(), 
        [](const std::pair<unsigned int, int>& a, const std::pair<unsigned int, int>& b) {
            return a.second > b.second;
        });
    
    int totalKeyPresses = 0;
    for (const auto& pair : sortedKeys) {
        std::string keyName = getKeyName(pair.first);
        oss << keyName << " (VK_" << pair.first << "): " << pair.second << std::endl;
        totalKeyPresses += pair.second;
    }
    oss << "TOTAL KEY PRESSES: " << totalKeyPresses << std::endl;
    oss << std::endl;
    
    oss << "=== End of Stats ===" << std::endl;
    
    return oss.str();
}

void BongoStats::saveStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (statsFilePath.empty()) {
        return;
    }
    
    try {
        std::ofstream outFile(statsFilePath);
        if (outFile.is_open()) {
            outFile << formatStats();
            outFile.flush();
            outFile.close();
        }
    } catch (...) {
        // Silently fail - stats saving shouldn't crash the app
    }
}

void BongoStats::loadStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (statsFilePath.empty()) {
        return;
    }
    
    try {
        std::ifstream inFile(statsFilePath);
        if (!inFile.is_open()) {
            return; // File doesn't exist yet, start fresh
        }
        
        std::string line;
        bool inMouseSection = false;
        bool inKeyboardSection = false;
        
        while (std::getline(inFile, line)) {
            // Skip header lines
            if (line.find("===") != std::string::npos || line.empty()) {
                continue;
            }
            
            // Detect sections
            if (line.find("Mouse Button") != std::string::npos) {
                inMouseSection = true;
                inKeyboardSection = false;
                continue;
            }
            if (line.find("Keyboard Key") != std::string::npos) {
                inMouseSection = false;
                inKeyboardSection = true;
                continue;
            }
            
            // Parse mouse button counts
            if (inMouseSection && line.find(":") != std::string::npos) {
                size_t colonPos = line.find(":");
                if (colonPos != std::string::npos) {
                    std::string buttonName = line.substr(0, colonPos);
                    // Trim whitespace
                    buttonName.erase(0, buttonName.find_first_not_of(" \t"));
                    buttonName.erase(buttonName.find_last_not_of(" \t") + 1);
                    
                    if (buttonName != "TOTAL MOUSE CLICKS") {
                        std::string countStr = line.substr(colonPos + 1);
                        countStr.erase(0, countStr.find_first_not_of(" \t"));
                        try {
                            int count = std::stoi(countStr);
                            mouseButtonCounts[buttonName] = count;
                        } catch (...) {
                            // Ignore parse errors
                        }
                    }
                }
            }
            
            // Parse keyboard key counts
            if (inKeyboardSection && line.find(":") != std::string::npos) {
                size_t colonPos = line.find(":");
                if (colonPos != std::string::npos) {
                    std::string keyPart = line.substr(0, colonPos);
                    // Extract VK code from "(VK_XXX)"
                    size_t vkStart = keyPart.find("(VK_");
                    if (vkStart != std::string::npos) {
                        size_t vkEnd = keyPart.find(")", vkStart);
                        if (vkEnd != std::string::npos) {
                            std::string vkStr = keyPart.substr(vkStart + 4, vkEnd - vkStart - 4);
                            try {
                                unsigned int keyCode = static_cast<unsigned int>(std::stoul(vkStr));
                                std::string countStr = line.substr(colonPos + 1);
                                countStr.erase(0, countStr.find_first_not_of(" \t"));
                                int count = std::stoi(countStr);
                                keyPressCounts[keyCode] = count;
                            } catch (...) {
                                // Ignore parse errors
                            }
                        }
                    }
                }
            }
        }
        inFile.close();
    } catch (...) {
        // On error, start with empty stats
    }
}


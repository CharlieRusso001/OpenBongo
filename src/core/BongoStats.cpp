#include "core/BongoStats.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <vector>
#include <mutex>
#include <cmath>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

void BongoStats::initialize(const std::string& statsFilePath) {
    std::lock_guard<std::mutex> lock(statsMutex);
    this->statsFilePath = statsFilePath;
    firstKeyPressTime = 0;
    lastKeyPressTime = 0;
    totalMinutesOpen = 0.0;
    // Load stats first to get saved totalMinutesOpen
    loadStats();
    // Then set app start time for current session
    appStartTime = std::time(nullptr);
}

void BongoStats::recordKeyPress(unsigned int keyCode) {
    std::lock_guard<std::mutex> lock(statsMutex);
    keyPressCounts[keyCode]++;
    
    // Record timestamp for KPM/WPM calculation
    time_t now = std::time(nullptr);
    keyPressTimestamps.push_back(now);
    
    // Keep only last 1000 timestamps to avoid memory issues
    if (keyPressTimestamps.size() > 1000) {
        keyPressTimestamps.erase(keyPressTimestamps.begin());
    }
    
    if (firstKeyPressTime == 0) {
        firstKeyPressTime = now;
    }
    lastKeyPressTime = now;
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
    
    // Total minutes open
    oss << "--- Total Minutes Open ---" << std::endl;
    oss << "TOTAL MINUTES OPEN: " << std::fixed << std::setprecision(2) << totalMinutesOpen << std::endl;
    oss << std::endl;
    
    oss << "=== End of Stats ===" << std::endl;
    
    return oss.str();
}

void BongoStats::saveStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (statsFilePath.empty()) {
        return;
    }
    
    // Update total minutes before saving to ensure current session time is included
    if (appStartTime > 0) {
        time_t now = std::time(nullptr);
        double minutesThisSession = static_cast<double>(now - appStartTime) / 60.0;
        if (minutesThisSession > 0.001) { // Update if any time has passed
            totalMinutesOpen += minutesThisSession;
            appStartTime = now; // Reset appStartTime to now for next interval
        }
    }
    
    try {
        // Helper function to escape JSON strings
        auto escapeJSON = [](const std::string& str) -> std::string {
            std::string escaped;
            for (char c : str) {
                if (c == '\\') escaped += "\\\\";
                else if (c == '"') escaped += "\\\"";
                else if (c == '\n') escaped += "\\n";
                else if (c == '\r') escaped += "\\r";
                else if (c == '\t') escaped += "\\t";
                else escaped += c;
            }
            return escaped;
        };
        
        std::ofstream outFile(statsFilePath);
        if (outFile.is_open()) {
            // Get current year for year-based stats
            time_t now = std::time(nullptr);
            std::tm* timeInfo = std::localtime(&now);
            int currentYear = 1900 + timeInfo->tm_year;
            
            // Format date string
            char dateStr[64];
            std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", timeInfo);
            
            outFile << "{\n";
            outFile << "  \"year\": " << currentYear << ",\n";
            outFile << "  \"lastUpdated\": \"" << dateStr << "\",\n";
            outFile << "  \"totalMinutesOpen\": " << std::fixed << std::setprecision(2) << totalMinutesOpen << ",\n";
            
            // Save mouse button counts
            outFile << "  \"mouseButtonCounts\": {\n";
            bool firstMouse = true;
            for (const auto& pair : mouseButtonCounts) {
                if (!firstMouse) outFile << ",\n";
                outFile << "    \"" << escapeJSON(pair.first) << "\": " << pair.second;
                firstMouse = false;
            }
            outFile << "\n  },\n";
            
            // Save keyboard key counts
            outFile << "  \"keyPressCounts\": {\n";
            bool firstKey = true;
            for (const auto& pair : keyPressCounts) {
                if (!firstKey) outFile << ",\n";
                outFile << "    \"" << pair.first << "\": " << pair.second;
                firstKey = false;
            }
            outFile << "\n  }\n";
            
            outFile << "}\n";
            outFile.flush();
            outFile.close();
        }
    } catch (...) {
        // Silently fail - stats saving shouldn't crash the app
    }
}

void BongoStats::loadStats(bool mergeWithCurrent) {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (statsFilePath.empty()) {
        return;
    }
    
    try {
        std::ifstream inFile(statsFilePath);
        if (!inFile.is_open()) {
            // File doesn't exist yet, start fresh
            return;
        }

        // Always load from file (don't merge) - clear existing data first
        if (!mergeWithCurrent) {
            keyPressCounts.clear();
            mouseButtonCounts.clear();
            totalMinutesOpen = 0.0;
            firstKeyPressTime = 0;
            lastKeyPressTime = 0;
        }
        
        // Read entire file into string
        std::string jsonContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        
        // Simple JSON parser - find values by key
        auto findJSONValue = [](const std::string& json, const std::string& key) -> std::string {
            std::string searchKey = "\"" + key + "\"";
            size_t pos = json.find(searchKey);
            if (pos == std::string::npos) return "";
            
            // Find the colon after the key
            pos = json.find(":", pos);
            if (pos == std::string::npos) return "";
            pos++; // Skip colon
            
            // Skip whitespace
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
            
            if (pos >= json.size()) return "";
            
            // If it's a string value (starts with ")
            if (json[pos] == '"') {
                pos++; // Skip opening quote
                size_t endPos = pos;
                while (endPos < json.size()) {
                    if (json[endPos] == '\\') {
                        endPos += 2; // Skip escaped character
                        continue;
                    }
                    if (json[endPos] == '"') {
                        break; // Found closing quote
                    }
                    endPos++;
                }
                if (endPos < json.size()) {
                    return json.substr(pos, endPos - pos);
                }
            } else {
                // Number or other value - find end (comma, }, or newline)
                size_t endPos = pos;
                while (endPos < json.size() && json[endPos] != ',' && json[endPos] != '}' && json[endPos] != '\n' && json[endPos] != ' ') {
                    endPos++;
                }
                std::string value = json.substr(pos, endPos - pos);
                // Trim trailing whitespace
                while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
                    value.pop_back();
                }
                return value;
            }
            return "";
        };
        
        auto findJSONObject = [](const std::string& json, const std::string& key) -> std::string {
            std::string searchKey = "\"" + key + "\"";
            size_t pos = json.find(searchKey);
            if (pos == std::string::npos) return "";
            
            // Find the colon after the key
            pos = json.find(":", pos);
            if (pos == std::string::npos) return "";
            pos++; // Skip colon
            
            // Skip whitespace
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
            
            if (pos >= json.size() || json[pos] != '{') return "";
            
            // Find matching closing brace
            int braceCount = 0;
            size_t startPos = pos;
            while (pos < json.size()) {
                if (json[pos] == '{') braceCount++;
                else if (json[pos] == '}') {
                    braceCount--;
                    if (braceCount == 0) {
                        return json.substr(startPos, pos - startPos + 1);
                    }
                } else if (json[pos] == '"') {
                    // Skip string content
                    pos++;
                    while (pos < json.size() && json[pos] != '"') {
                        if (json[pos] == '\\') pos++; // Skip escaped character
                        pos++;
                    }
                }
                pos++;
            }
            return "";
        };
        
        // Check year - if different year, reset stats
        std::string yearStr = findJSONValue(jsonContent, "year");
        if (!yearStr.empty()) {
            try {
                int fileYear = std::stoi(yearStr);
                time_t now = std::time(nullptr);
                std::tm* ti = std::localtime(&now);
                int currentYear = ti ? (1900 + ti->tm_year) : fileYear;
                if (fileYear != currentYear) {
                    // Year changed: reset stats
                    keyPressCounts.clear();
                    mouseButtonCounts.clear();
                    totalMinutesOpen = 0.0;
                    firstKeyPressTime = 0;
                    lastKeyPressTime = 0;
                    appStartTime = std::time(nullptr);
                    // Clear the file
                    std::ofstream out(statsFilePath, std::ios::trunc);
                    out << "{\n  \"year\": " << currentYear << ",\n  \"totalMinutesOpen\": 0.0,\n  \"mouseButtonCounts\": {},\n  \"keyPressCounts\": {}\n}\n";
                    return;
                }
            } catch (...) {
                // Ignore parse errors
            }
        }
        
        // Load total minutes open
        std::string minutesStr = findJSONValue(jsonContent, "totalMinutesOpen");
        if (!minutesStr.empty()) {
            try {
                totalMinutesOpen = std::stod(minutesStr);
            } catch (...) {
                // Ignore parse errors
            }
        }
        
        // Load mouse button counts
        std::string mouseObj = findJSONObject(jsonContent, "mouseButtonCounts");
        if (!mouseObj.empty()) {
            // Parse key-value pairs from the object
            size_t pos = 1; // Skip opening {
            while (pos < mouseObj.size() - 1) {
                // Skip whitespace and commas
                while (pos < mouseObj.size() && (mouseObj[pos] == ' ' || mouseObj[pos] == '\t' || mouseObj[pos] == ',' || mouseObj[pos] == '\n')) pos++;
                if (pos >= mouseObj.size() - 1) break;
                
                // Find key (string in quotes)
                if (mouseObj[pos] != '"') break;
                pos++; // Skip opening quote
                size_t keyStart = pos;
                while (pos < mouseObj.size() && mouseObj[pos] != '"') {
                    if (mouseObj[pos] == '\\') pos++; // Skip escaped character
                    pos++;
                }
                std::string buttonName = mouseObj.substr(keyStart, pos - keyStart);
                pos++; // Skip closing quote
                
                // Find colon
                while (pos < mouseObj.size() && mouseObj[pos] != ':') pos++;
                if (pos >= mouseObj.size()) break;
                pos++; // Skip colon
                
                // Skip whitespace
                while (pos < mouseObj.size() && (mouseObj[pos] == ' ' || mouseObj[pos] == '\t')) pos++;
                
                // Find number value
                size_t valueStart = pos;
                while (pos < mouseObj.size() && mouseObj[pos] != ',' && mouseObj[pos] != '}' && mouseObj[pos] != ' ' && mouseObj[pos] != '\n') pos++;
                std::string countStr = mouseObj.substr(valueStart, pos - valueStart);
                try {
                    int count = std::stoi(countStr);
                    if (!mergeWithCurrent) {
                        mouseButtonCounts[buttonName] = count;
                    } else {
                        mouseButtonCounts[buttonName] += count;
                    }
                } catch (...) {
                    // Ignore parse errors
                }
            }
        }
        
        // Load keyboard key counts
        std::string keyObj = findJSONObject(jsonContent, "keyPressCounts");
        if (!keyObj.empty()) {
            // Parse key-value pairs from the object
            size_t pos = 1; // Skip opening {
            while (pos < keyObj.size() - 1) {
                // Skip whitespace and commas
                while (pos < keyObj.size() && (keyObj[pos] == ' ' || keyObj[pos] == '\t' || keyObj[pos] == ',' || keyObj[pos] == '\n')) pos++;
                if (pos >= keyObj.size() - 1) break;
                
                // Find key (number as string in quotes, or just number)
                size_t keyStart = pos;
                if (keyObj[pos] == '"') {
                    pos++; // Skip opening quote
                    keyStart = pos;
                    while (pos < keyObj.size() && keyObj[pos] != '"') pos++;
                } else {
                    while (pos < keyObj.size() && keyObj[pos] != ':' && keyObj[pos] != ' ' && keyObj[pos] != '\t') pos++;
                }
                std::string keyCodeStr = keyObj.substr(keyStart, pos - keyStart);
                if (keyObj[pos] == '"') pos++; // Skip closing quote
                
                // Find colon
                while (pos < keyObj.size() && keyObj[pos] != ':') pos++;
                if (pos >= keyObj.size()) break;
                pos++; // Skip colon
                
                // Skip whitespace
                while (pos < keyObj.size() && (keyObj[pos] == ' ' || keyObj[pos] == '\t')) pos++;
                
                // Find number value
                size_t valueStart = pos;
                while (pos < keyObj.size() && keyObj[pos] != ',' && keyObj[pos] != '}' && keyObj[pos] != ' ' && keyObj[pos] != '\n') pos++;
                std::string countStr = keyObj.substr(valueStart, pos - valueStart);
                try {
                    unsigned int keyCode = static_cast<unsigned int>(std::stoul(keyCodeStr));
                    int count = std::stoi(countStr);
                    if (!mergeWithCurrent) {
                        keyPressCounts[keyCode] = count;
                    } else {
                        keyPressCounts[keyCode] += count;
                    }
                } catch (...) {
                    // Ignore parse errors
                }
            }
        }
        
        // After loading, reset appStartTime to now (file already contains accumulated time)
        appStartTime = std::time(nullptr);
    } catch (...) {
        // On error, start with empty stats
    }
}

std::map<std::string, int> BongoStats::getAllKeyStats() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    std::map<std::string, int> result;
    for (const auto& pair : keyPressCounts) {
        result[getKeyName(pair.first)] = pair.second;
    }
    return result;
}

int BongoStats::getTotalKeyPresses() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    int total = 0;
    for (const auto& pair : keyPressCounts) {
        total += pair.second;
    }
    return total;
}

double BongoStats::getKeysPerMinute() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (keyPressTimestamps.empty() || firstKeyPressTime == 0) {
        return 0.0;
    }
    
    time_t timeDiff = lastKeyPressTime - firstKeyPressTime;
    if (timeDiff <= 0) {
        return 0.0;
    }
    
    double minutes = static_cast<double>(timeDiff) / 60.0;
    if (minutes <= 0) {
        return 0.0;
    }
    
    return static_cast<double>(keyPressTimestamps.size()) / minutes;
}

double BongoStats::getWordsPerMinute() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    // Estimate: 5 characters = 1 word
    // Count only letter keys (A-Z) for WPM calculation
    int letterKeyCount = 0;
    for (const auto& pair : keyPressCounts) {
        unsigned int keyCode = pair.first;
        if (keyCode >= 'A' && keyCode <= 'Z') {
            letterKeyCount += pair.second;
        }
    }
    
    if (keyPressTimestamps.empty() || firstKeyPressTime == 0) {
        return 0.0;
    }
    
    time_t timeDiff = lastKeyPressTime - firstKeyPressTime;
    if (timeDiff <= 0) {
        return 0.0;
    }
    
    double minutes = static_cast<double>(timeDiff) / 60.0;
    if (minutes <= 0) {
        return 0.0;
    }
    
    // 5 characters = 1 word
    double words = static_cast<double>(letterKeyCount) / 5.0;
    return words / minutes;
}

std::string BongoStats::getWrappedStatsJSON() const {
    // Note: We don't lock at the start because getTotalKeyPresses(), getKeysPerMinute(), 
    // and getWordsPerMinute() already lock the mutex themselves. We only lock when
    // directly accessing keyPressCounts.
    
    // Helper function to escape JSON strings
    auto escapeJSON = [](const std::string& str) -> std::string {
        std::string escaped;
        for (char c : str) {
            if (c == '\\') escaped += "\\\\";
            else if (c == '"') escaped += "\\\"";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\t') escaped += "\\t";
            else escaped += c;
        }
        return escaped;
    };
    
    std::ostringstream json;
    json << "{";
    
    // Get current year
    std::time_t now = std::time(nullptr);
    std::tm* timeInfo = std::localtime(&now);
    int currentYear = 1900 + timeInfo->tm_year;
    
    json << "\"year\":" << currentYear << ",";
    // Get total keys and mouse clicks
    int totalKeys = getTotalKeyPresses();
    int totalMouseClicks = 0;
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        for (const auto& pair : mouseButtonCounts) {
            totalMouseClicks += pair.second;
        }
    }
    int totalInputs = totalKeys + totalMouseClicks;
    
    json << "\"totalKeys\":" << totalKeys << ",";
    json << "\"totalMouseClicks\":" << totalMouseClicks << ",";
    json << "\"totalInputs\":" << totalInputs << ",";
    json << "\"keysPerMinute\":" << std::fixed << std::setprecision(2) << getKeysPerMinute() << ",";
    json << "\"wordsPerMinute\":" << std::fixed << std::setprecision(2) << getWordsPerMinute() << ",";
    json << "\"totalMinutesOpen\":" << std::fixed << std::setprecision(2) << getTotalMinutesOpen() << ",";
    
    // Get top keys and mouse clicks combined - need to lock here to access both maps
    std::vector<std::pair<std::string, int>> sortedInputs;
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        // Add keys
        for (const auto& pair : keyPressCounts) {
            sortedInputs.push_back({getKeyName(pair.first), pair.second});
        }
        // Add mouse clicks
        for (const auto& pair : mouseButtonCounts) {
            sortedInputs.push_back({pair.first + " CLICK", pair.second});
        }
    }
    std::sort(sortedInputs.begin(), sortedInputs.end(), 
        [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            return a.second > b.second;
        });
    
    json << "\"topInputs\":[";
    int topCount = (sortedInputs.size() < 10) ? static_cast<int>(sortedInputs.size()) : 10;
    for (int i = 0; i < topCount; i++) {
        if (i > 0) json << ",";
        json << "{\"key\":\"" << escapeJSON(sortedInputs[i].first) << "\",\"count\":" << sortedInputs[i].second << "}";
    }
    json << "]";
    
    json << "}";
    return json.str();
}

double BongoStats::getTotalMinutesOpen() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    // Calculate current session minutes and add to saved total
    // Note: appStartTime is the start of the current session (or last update)
    if (appStartTime > 0) {
        time_t now = std::time(nullptr);
        double minutesThisSession = static_cast<double>(now - appStartTime) / 60.0;
        // Return saved total plus current session time
        return totalMinutesOpen + minutesThisSession;
    }
    return totalMinutesOpen;
}

void BongoStats::setAppStartTime(time_t startTime) {
    std::lock_guard<std::mutex> lock(statsMutex);
    appStartTime = startTime;
}

void BongoStats::updateTotalMinutes() {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (appStartTime > 0) {
        time_t now = std::time(nullptr);
        double minutesThisSession = static_cast<double>(now - appStartTime) / 60.0;
        if (minutesThisSession > 0.001) { // Update if at least 0.001 minutes (0.06 seconds) have passed
            totalMinutesOpen += minutesThisSession;
            // Update appStartTime to now to track next interval (prevents double-counting)
            appStartTime = now;
        }
    } else {
        // If appStartTime is 0, set it to now to start tracking
        appStartTime = std::time(nullptr);
    }
}


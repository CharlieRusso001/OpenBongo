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
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

void BongoStats::initialize(const std::string& baseDataDir) {
    std::lock_guard<std::mutex> lock(statsMutex);
    this->baseDataDir = baseDataDir;
    firstKeyPressTime = 0;
    lastKeyPressTime = 0;
    // Don't set totalMinutesOpen to 0 - let loadStats set it from file
    // Load today's stats from file (this will set totalMinutesOpen from file)
    loadStats(false); // Load from today's file
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

// Helper: Get today's file path (e.g., DATA/2025/12.26.25.json)
std::string BongoStats::getTodayFilePath() const {
    time_t now = std::time(nullptr);
    std::tm* timeInfo = std::localtime(&now);
    int year = 1900 + timeInfo->tm_year;
    int month = timeInfo->tm_mon + 1;
    int day = timeInfo->tm_mday;
    int yearShort = year % 100;
    
    std::string yearFolder = getYearFolderPath(year);
    ensureYearFolderExists(year);
    
    // Format: MM.DD.YY.json (e.g., 12.26.25.json)
    std::ostringstream filename;
    filename << std::setfill('0') << std::setw(2) << month << "."
             << std::setw(2) << day << "."
             << std::setw(2) << yearShort << ".json";
    
    return (std::filesystem::path(yearFolder) / filename.str()).string();
}

// Helper: Get year folder path
std::string BongoStats::getYearFolderPath(int year) const {
    return (std::filesystem::path(baseDataDir) / "DATA" / std::to_string(year)).string();
}

// Helper: Ensure year folder exists
void BongoStats::ensureYearFolderExists(int year) const {
    if (baseDataDir.empty()) return;
    try {
        std::string dataDir = (std::filesystem::path(baseDataDir) / "DATA").string();
        std::filesystem::create_directories(dataDir);
        std::string yearFolder = getYearFolderPath(year);
        std::filesystem::create_directories(yearFolder);
    } catch (...) {
        // Silently fail
    }
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
    if (baseDataDir.empty()) {
        return;
    }
    
    try {
        // Get today's file path
        std::string todayFile = getTodayFilePath();
        
        // CRITICAL: Load existing data from file first and merge with current session data
        // This ensures we never lose data that's already in the file
        std::map<unsigned int, int> existingKeyCounts;
        std::map<std::string, int> existingMouseCounts;
        double existingTotalMinutes = 0.0;
        
        // Try to load existing data from today's file
        std::map<unsigned int, int> tempKeyCounts;
        std::map<std::string, int> tempMouseCounts;
        double tempMinutes = 0.0;
        bool fileExists = parseDailyFile(todayFile, tempKeyCounts, tempMouseCounts, tempMinutes);
        if (fileExists) {
            existingKeyCounts = tempKeyCounts;
            existingMouseCounts = tempMouseCounts;
            existingTotalMinutes = tempMinutes;
        }
        
        // CRITICAL SAFETY CHECK: If file exists and has data, but in-memory state is empty,
        // DO NOT SAVE - this would overwrite the file with empty data
        // This prevents data loss when app closes with empty in-memory state
        int existingTotal = 0;
        for (const auto& pair : existingKeyCounts) existingTotal += pair.second;
        for (const auto& pair : existingMouseCounts) existingTotal += pair.second;
        
        int currentTotal = 0;
        for (const auto& pair : keyPressCounts) currentTotal += pair.second;
        for (const auto& pair : mouseButtonCounts) currentTotal += pair.second;
        
        if (fileExists && existingTotal > 0 && currentTotal == 0) {
            // File has data but in-memory is completely empty - don't overwrite!
            // This happens when loadStats fails or file doesn't load properly
            return;
        }
        
        // Update total minutes before saving to ensure current session time is included
        // But only if we have actual activity or existing data
        if (appStartTime > 0 && (currentTotal > 0 || existingTotal > 0)) {
            time_t now = std::time(nullptr);
            double minutesThisSession = static_cast<double>(now - appStartTime) / 60.0;
            if (minutesThisSession > 0.001) { // Update if any time has passed
                totalMinutesOpen += minutesThisSession;
                appStartTime = now; // Reset appStartTime to now for next interval
            }
        }
        
        // Merge: Start with existing file data, then add current session's new data
        // This ensures we never lose data that's already saved
        std::map<unsigned int, int> mergedKeyCounts = existingKeyCounts;
        for (const auto& pair : keyPressCounts) {
            // Add to existing count (if key exists) or set to current value
            // Since keyPressCounts might already include loaded data, we use the maximum
            // to avoid double-counting while preserving all data
            if (mergedKeyCounts.find(pair.first) == mergedKeyCounts.end()) {
                mergedKeyCounts[pair.first] = pair.second;
            } else {
                // Use maximum to ensure we never lose data
                if (pair.second > mergedKeyCounts[pair.first]) {
                    mergedKeyCounts[pair.first] = pair.second;
                }
            }
        }
        
        std::map<std::string, int> mergedMouseCounts = existingMouseCounts;
        for (const auto& pair : mouseButtonCounts) {
            // Add to existing count (if key exists) or set to current value
            if (mergedMouseCounts.find(pair.first) == mergedMouseCounts.end()) {
                mergedMouseCounts[pair.first] = pair.second;
            } else {
                // Use maximum to ensure we never lose data
                if (pair.second > mergedMouseCounts[pair.first]) {
                    mergedMouseCounts[pair.first] = pair.second;
                }
            }
        }
        
        // For total minutes, use the maximum to ensure we never lose time
        double mergedTotalMinutes = (existingTotalMinutes > totalMinutesOpen) ? existingTotalMinutes : totalMinutesOpen;
        
        // SAFETY CHECK: Never overwrite a file that has data with an empty or smaller dataset
        // This prevents data loss if saveStats() is called with empty in-memory state
        int existingTotalKeys = 0;
        for (const auto& pair : existingKeyCounts) {
            existingTotalKeys += pair.second;
        }
        int existingTotalMouse = 0;
        for (const auto& pair : existingMouseCounts) {
            existingTotalMouse += pair.second;
        }
        
        int currentTotalKeys = 0;
        for (const auto& pair : keyPressCounts) {
            currentTotalKeys += pair.second;
        }
        int currentTotalMouse = 0;
        for (const auto& pair : mouseButtonCounts) {
            currentTotalMouse += pair.second;
        }
        
        // CRITICAL: If existing file has data and current state would result in less data,
        // use the merged data (which preserves existing) instead of overwriting
        // Only skip saving entirely if existing file has substantial data and current is completely empty
        // (This might happen if saveStats is called before loadStats)
        if (existingTotalKeys + existingTotalMouse > 0 && 
            currentTotalKeys + currentTotalMouse == 0 &&
            mergedKeyCounts.size() == existingKeyCounts.size() &&
            mergedMouseCounts.size() == existingMouseCounts.size()) {
            // File has data, in-memory is empty, and merge didn't add anything new
            // Don't overwrite - preserve existing file
            return;
        }
        
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
        
        // FINAL SAFETY CHECK: Verify merged data has at least as much as existing file
        int mergedTotalKeys = 0;
        for (const auto& pair : mergedKeyCounts) {
            mergedTotalKeys += pair.second;
        }
        int mergedTotalMouse = 0;
        for (const auto& pair : mergedMouseCounts) {
            mergedTotalMouse += pair.second;
        }
        
        // CRITICAL: If file exists and has data, never save less data than what's in the file
        // This is the ultimate protection against data loss
        if (fileExists && (existingTotalKeys + existingTotalMouse > 0)) {
            int mergedTotal = mergedTotalKeys + mergedTotalMouse;
            int existingTotal = existingTotalKeys + existingTotalMouse;
            
            // If merged data has less than existing file, something is wrong - don't save
            if (mergedTotal < existingTotal) {
                // Merged data has less than existing - this shouldn't happen, but protect against it
                // This prevents any scenario where we'd lose data
                return;
            }
            
            // Also check individual counts - if any key/mouse count would decrease, don't save
            // This is extra protection
            for (const auto& existingPair : existingKeyCounts) {
                auto mergedIt = mergedKeyCounts.find(existingPair.first);
                if (mergedIt != mergedKeyCounts.end() && mergedIt->second < existingPair.second) {
                    // This key would have less count - don't save
                    return;
                }
            }
            for (const auto& existingPair : existingMouseCounts) {
                auto mergedIt = mergedMouseCounts.find(existingPair.first);
                if (mergedIt != mergedMouseCounts.end() && mergedIt->second < existingPair.second) {
                    // This mouse button would have less count - don't save
                    return;
                }
            }
        }
        
        std::ofstream outFile(todayFile);
        if (outFile.is_open()) {
            // Get current year and date
            time_t now = std::time(nullptr);
            std::tm* timeInfo = std::localtime(&now);
            int currentYear = 1900 + timeInfo->tm_year;
            
            // Format date string
            char dateStr[64];
            std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M:%S", timeInfo);
            
            outFile << "{\n";
            outFile << "  \"year\": " << currentYear << ",\n";
            outFile << "  \"date\": \"" << dateStr << "\",\n";
            outFile << "  \"totalMinutesOpen\": " << std::fixed << std::setprecision(2) << mergedTotalMinutes << ",\n";
            
            // Save merged mouse button counts
            outFile << "  \"mouseButtonCounts\": {\n";
            bool firstMouse = true;
            for (const auto& pair : mergedMouseCounts) {
                if (!firstMouse) outFile << ",\n";
                outFile << "    \"" << escapeJSON(pair.first) << "\": " << pair.second;
                firstMouse = false;
            }
            outFile << "\n  },\n";
            
            // Save merged keyboard key counts
            outFile << "  \"keyPressCounts\": {\n";
            bool firstKey = true;
            for (const auto& pair : mergedKeyCounts) {
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

// Parse a single daily JSON file
bool BongoStats::parseDailyFile(const std::string& filePath, std::map<unsigned int, int>& keyCounts, 
                                  std::map<std::string, int>& mouseCounts, double& minutes) const {
    try {
        std::ifstream inFile(filePath);
        if (!inFile.is_open()) {
            return false;
        }
        
        std::string jsonContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        
        if (jsonContent.empty()) return false;
        
        // Helper to find JSON value
        auto findJSONValue = [](const std::string& json, const std::string& key) -> std::string {
            std::string searchKey = "\"" + key + "\"";
            size_t pos = json.find(searchKey);
            if (pos == std::string::npos) return "";
            pos = json.find(":", pos);
            if (pos == std::string::npos) return "";
            pos++;
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
            if (pos >= json.size()) return "";
            if (json[pos] == '"') {
                pos++;
                size_t endPos = pos;
                while (endPos < json.size()) {
                    if (json[endPos] == '\\') { endPos += 2; continue; }
                    if (json[endPos] == '"') break;
                    endPos++;
                }
                if (endPos < json.size()) {
                    return json.substr(pos, endPos - pos);
                }
            } else {
                size_t endPos = pos;
                while (endPos < json.size() && json[endPos] != ',' && json[endPos] != '}' && json[endPos] != '\n' && json[endPos] != ' ') {
                    endPos++;
                }
                std::string value = json.substr(pos, endPos - pos);
                while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
                    value.pop_back();
                }
                return value;
            }
            return "";
        };
        
        // Helper to find JSON object
        auto findJSONObject = [](const std::string& json, const std::string& key) -> std::string {
            std::string searchKey = "\"" + key + "\"";
            size_t pos = json.find(searchKey);
            if (pos == std::string::npos) return "";
            pos = json.find(":", pos);
            if (pos == std::string::npos) return "";
            pos++;
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
            if (pos >= json.size() || json[pos] != '{') return "";
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
                    pos++;
                    while (pos < json.size() && json[pos] != '"') {
                        if (json[pos] == '\\') pos++;
                        pos++;
                    }
                }
                pos++;
            }
            return "";
        };
        
        // Get total minutes
        std::string minutesStr = findJSONValue(jsonContent, "totalMinutesOpen");
        if (!minutesStr.empty()) {
            try {
                minutes += std::stod(minutesStr); // Add to existing (for aggregation)
            } catch (...) {}
        }
        
        // Parse mouse button counts
        std::string mouseObj = findJSONObject(jsonContent, "mouseButtonCounts");
        if (!mouseObj.empty()) {
            size_t pos = 1;
            while (pos < mouseObj.size() - 1) {
                while (pos < mouseObj.size() && (mouseObj[pos] == ' ' || mouseObj[pos] == '\t' || mouseObj[pos] == ',' || mouseObj[pos] == '\n' || mouseObj[pos] == '\r')) pos++;
                if (pos >= mouseObj.size() - 1 || mouseObj[pos] == '}') break;
                if (mouseObj[pos] != '"') { pos++; continue; }
                pos++;
                size_t keyStart = pos;
                while (pos < mouseObj.size() && mouseObj[pos] != '"') {
                    if (mouseObj[pos] == '\\' && pos + 1 < mouseObj.size()) { pos += 2; continue; }
                    pos++;
                }
                if (pos >= mouseObj.size()) break;
                std::string buttonName = mouseObj.substr(keyStart, pos - keyStart);
                pos++;
                while (pos < mouseObj.size() && mouseObj[pos] != ':') pos++;
                if (pos >= mouseObj.size()) break;
                pos++;
                while (pos < mouseObj.size() && (mouseObj[pos] == ' ' || mouseObj[pos] == '\t')) pos++;
                size_t valueStart = pos;
                while (pos < mouseObj.size() && mouseObj[pos] != ',' && mouseObj[pos] != '}' && mouseObj[pos] != '\n' && mouseObj[pos] != '\r') pos++;
                std::string countStr = mouseObj.substr(valueStart, pos - valueStart);
                while (!countStr.empty() && (countStr.back() == ' ' || countStr.back() == '\t' || countStr.back() == '\r' || countStr.back() == '\n')) {
                    countStr.pop_back();
                }
                while (!countStr.empty() && (countStr.front() == ' ' || countStr.front() == '\t')) {
                    countStr.erase(0, 1);
                }
                try {
                    if (!countStr.empty()) {
                        int count = std::stoi(countStr);
                        mouseCounts[buttonName] += count; // Add to existing (for aggregation)
                    }
                } catch (...) {}
                if (pos < mouseObj.size() && mouseObj[pos] == ',') pos++;
            }
        }
        
        // Parse keyboard key counts
        std::string keyObj = findJSONObject(jsonContent, "keyPressCounts");
        if (!keyObj.empty()) {
            size_t pos = 1;
            while (pos < keyObj.size() - 1) {
                while (pos < keyObj.size() && (keyObj[pos] == ' ' || keyObj[pos] == '\t' || keyObj[pos] == ',' || keyObj[pos] == '\n')) pos++;
                if (pos >= keyObj.size() - 1 || keyObj[pos] == '}') break;
                size_t keyStart = pos;
                if (keyObj[pos] == '"') {
                    pos++;
                    keyStart = pos;
                    while (pos < keyObj.size() && keyObj[pos] != '"') pos++;
                } else {
                    while (pos < keyObj.size() && keyObj[pos] != ':' && keyObj[pos] != ' ' && keyObj[pos] != '\t') pos++;
                }
                std::string keyCodeStr = keyObj.substr(keyStart, pos - keyStart);
                if (keyObj[pos] == '"') pos++;
                while (pos < keyObj.size() && keyObj[pos] != ':') pos++;
                if (pos >= keyObj.size()) break;
                pos++;
                while (pos < keyObj.size() && (keyObj[pos] == ' ' || keyObj[pos] == '\t')) pos++;
                size_t valueStart = pos;
                while (pos < keyObj.size() && keyObj[pos] != ',' && keyObj[pos] != '}' && keyObj[pos] != ' ' && keyObj[pos] != '\n') pos++;
                std::string countStr = keyObj.substr(valueStart, pos - valueStart);
                while (!countStr.empty() && (countStr.back() == ' ' || countStr.back() == '\t')) {
                    countStr.pop_back();
                }
                try {
                    if (!keyCodeStr.empty() && !countStr.empty()) {
                        unsigned int keyCode = static_cast<unsigned int>(std::stoul(keyCodeStr));
                        int count = std::stoi(countStr);
                        keyCounts[keyCode] += count; // Add to existing (for aggregation)
                    }
                } catch (...) {}
                if (pos < keyObj.size() && keyObj[pos] == ',') pos++;
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

void BongoStats::loadStats(bool mergeWithCurrent) {
    std::lock_guard<std::mutex> lock(statsMutex);
    if (baseDataDir.empty()) {
        return;
    }
    
    // Load from today's file only
    std::string todayFile = getTodayFilePath();
    
    try {
        std::ifstream inFile(todayFile);
        if (!inFile.is_open()) {
            // File doesn't exist yet - only clear if explicitly requested
            // Don't clear if we're just initializing and file doesn't exist
            if (!mergeWithCurrent) {
                // Only clear if we're explicitly loading (not just initializing)
                // But preserve existing in-memory state if file doesn't exist
                // This prevents overwriting with empty data
            }
            return;
        }

        // Always load from file (don't merge) - clear existing data first
        if (!mergeWithCurrent) {
            keyPressCounts.clear();
            mouseButtonCounts.clear();
            // Don't reset totalMinutesOpen here - let parseDailyFile set it
            // If parseDailyFile fails, we'll keep whatever was there before
            firstKeyPressTime = 0;
            lastKeyPressTime = 0;
            // Don't clear keyPressTimestamps - they're for current session KPM/WPM calculation
        }
        
        // Use parseDailyFile to load today's data
        double fileMinutes = 0.0;
        std::map<unsigned int, int> fileKeyCounts;
        std::map<std::string, int> fileMouseCounts;
        
        if (parseDailyFile(todayFile, fileKeyCounts, fileMouseCounts, fileMinutes)) {
            if (!mergeWithCurrent) {
                keyPressCounts = fileKeyCounts;
                mouseButtonCounts = fileMouseCounts;
                totalMinutesOpen = fileMinutes;
            } else {
                // Merge with existing
                for (const auto& pair : fileKeyCounts) {
                    keyPressCounts[pair.first] += pair.second;
                }
                for (const auto& pair : fileMouseCounts) {
                    mouseButtonCounts[pair.first] += pair.second;
                }
                totalMinutesOpen += fileMinutes;
            }
        }
        // If parseDailyFile fails, we don't modify totalMinutesOpen
        // This preserves any existing value and prevents overwriting with 0
        
        // After loading, reset appStartTime to now (file already contains accumulated time)
        appStartTime = std::time(nullptr);
    } catch (...) {
        // On error, don't modify anything - preserve existing state
        // This prevents overwriting with empty data on errors
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
    // Read ALL daily files from the current year's folder and aggregate them
    // This ensures wrapped stats show the complete year's data
    
    // Lock to safely read the base directory
    std::string dataDir;
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        dataDir = baseDataDir;
    }
    
    // Get current year
    std::time_t now = std::time(nullptr);
    std::tm* timeInfo = std::localtime(&now);
    int currentYear = 1900 + timeInfo->tm_year;
    
    // If base directory is not set, return empty stats
    if (dataDir.empty()) {
        return "{\"year\":" + std::to_string(currentYear) + ",\"totalKeys\":0,\"totalMouseClicks\":0,\"totalInputs\":0,\"keysPerMinute\":0.00,\"wordsPerMinute\":0.00,\"totalMinutesOpen\":0.00,\"topInputs\":[]}";
    }
    
    // Get year folder path
    std::string yearFolder = getYearFolderPath(currentYear);
    
    // Aggregate data from all daily files in the year folder
    std::map<unsigned int, int> aggregatedKeyCounts;
    std::map<std::string, int> aggregatedMouseCounts;
    double aggregatedTotalMinutes = 0.0;
    
    // Iterate through all files in the year folder
    try {
        if (std::filesystem::exists(yearFolder) && std::filesystem::is_directory(yearFolder)) {
            for (const auto& entry : std::filesystem::directory_iterator(yearFolder)) {
                if (entry.is_regular_file()) {
                    std::string filePath = entry.path().string();
                    // Only process .json files
                    if (filePath.size() > 5 && filePath.substr(filePath.size() - 5) == ".json") {
                        // Parse this daily file and aggregate the data
                        std::map<unsigned int, int> fileKeyCounts;
                        std::map<std::string, int> fileMouseCounts;
                        double fileMinutes = 0.0;
                        if (parseDailyFile(filePath, fileKeyCounts, fileMouseCounts, fileMinutes)) {
                            // Aggregate key counts
                            for (const auto& pair : fileKeyCounts) {
                                aggregatedKeyCounts[pair.first] += pair.second;
                            }
                            // Aggregate mouse counts
                            for (const auto& pair : fileMouseCounts) {
                                aggregatedMouseCounts[pair.first] += pair.second;
                            }
                            // Aggregate minutes
                            aggregatedTotalMinutes += fileMinutes;
                        }
                    }
                }
            }
        }
    } catch (...) {
        // If folder doesn't exist or can't be read, continue with empty stats
    }
    
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
    
    // Calculate totals from aggregated data
    int totalKeys = 0;
    for (const auto& pair : aggregatedKeyCounts) {
        totalKeys += pair.second;
    }
    int totalMouseClicks = 0;
    for (const auto& pair : aggregatedMouseCounts) {
        totalMouseClicks += pair.second;
    }
    int totalInputs = totalKeys + totalMouseClicks;
    
    // Build sorted inputs list from aggregated data
    std::vector<std::pair<std::string, int>> sortedInputs;
    for (const auto& pair : aggregatedKeyCounts) {
        sortedInputs.push_back({getKeyName(pair.first), pair.second});
    }
    for (const auto& pair : aggregatedMouseCounts) {
        sortedInputs.push_back({pair.first + " CLICK", pair.second});
    }
    std::sort(sortedInputs.begin(), sortedInputs.end(), 
        [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            return a.second > b.second;
        });
    
    // Build JSON response
    std::ostringstream json;
    json << "{";
    json << "\"year\":" << currentYear << ",";
    json << "\"totalKeys\":" << totalKeys << ",";
    json << "\"totalMouseClicks\":" << totalMouseClicks << ",";
    json << "\"totalInputs\":" << totalInputs << ",";
    json << "\"keysPerMinute\":0.00,";
    json << "\"wordsPerMinute\":0.00,";
    json << "\"totalMinutesOpen\":" << std::fixed << std::setprecision(2) << aggregatedTotalMinutes << ",";
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


#ifndef BONGO_STATS_H
#define BONGO_STATS_H

#include <string>
#include <map>
#include <mutex>

class BongoStats {
public:
    static BongoStats& getInstance() {
        static BongoStats instance;
        return instance;
    }
    
    // Initialize stats file path
    void initialize(const std::string& statsFilePath);
    
    // Record a key press
    void recordKeyPress(unsigned int keyCode);
    
    // Record a mouse button click
    void recordMouseClick(const std::string& buttonName); // "LEFT", "RIGHT", "MIDDLE"
    
    // Save stats to file
    void saveStats();
    
    // Load stats from file
    void loadStats();
    
    // Get count for a specific key (returns key name or "Unknown")
    int getKeyCount(unsigned int keyCode) const;
    
    // Get count for a mouse button
    int getMouseButtonCount(const std::string& buttonName) const;
    
    ~BongoStats() {
        saveStats();
    }
    
private:
    BongoStats() = default;
    BongoStats(const BongoStats&) = delete;
    BongoStats& operator=(const BongoStats&) = delete;
    
    std::string statsFilePath;
    std::map<unsigned int, int> keyPressCounts; // keyCode -> count
    std::map<std::string, int> mouseButtonCounts; // "LEFT", "RIGHT", "MIDDLE" -> count
    mutable std::mutex statsMutex; // Thread-safe access
    
    // Convert Windows virtual key code to readable name
    std::string getKeyName(unsigned int keyCode) const;
    
    // Format stats for saving
    std::string formatStats() const;
};

#endif // BONGO_STATS_H


#ifndef BONGO_STATS_H
#define BONGO_STATS_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <ctime>

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
    void loadStats(bool mergeWithCurrent = false);
    
    // Get count for a specific key (returns key name or "Unknown")
    int getKeyCount(unsigned int keyCode) const;
    
    // Get count for a mouse button
    int getMouseButtonCount(const std::string& buttonName) const;
    
    // Get all key stats for wrapped
    std::map<std::string, int> getAllKeyStats() const;
    
    // Get total key presses
    int getTotalKeyPresses() const;
    
    // Get keys per minute (average)
    double getKeysPerMinute() const;
    
    // Get words per minute (estimated, 5 chars = 1 word)
    double getWordsPerMinute() const;
    
    // Get wrapped stats for current year
    std::string getWrappedStatsJSON() const;
    
    // Get total minutes open
    double getTotalMinutesOpen() const;
    
    // Set app start time (called when app starts)
    void setAppStartTime(time_t startTime);
    
    // Update total minutes (called periodically or on shutdown)
    void updateTotalMinutes();
    
    ~BongoStats() {
        updateTotalMinutes();
        saveStats();
    }
    
private:
    BongoStats() = default;
    BongoStats(const BongoStats&) = delete;
    BongoStats& operator=(const BongoStats&) = delete;
    
    std::string statsFilePath;
    std::map<unsigned int, int> keyPressCounts; // keyCode -> count
    std::map<std::string, int> mouseButtonCounts; // "LEFT", "RIGHT", "MIDDLE" -> count
    std::vector<time_t> keyPressTimestamps; // Timestamps for KPM/WPM calculation
    time_t firstKeyPressTime; // First key press time
    time_t lastKeyPressTime; // Last key press time
    time_t appStartTime; // When the app started (for total minutes tracking)
    double totalMinutesOpen; // Total minutes the app has been open (accumulated)
    mutable std::mutex statsMutex; // Thread-safe access
    
    // Convert Windows virtual key code to readable name
    std::string getKeyName(unsigned int keyCode) const;
    
    // Format stats for saving
    std::string formatStats() const;
};

#endif // BONGO_STATS_H


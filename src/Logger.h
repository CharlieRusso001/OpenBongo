#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <deque>
#include <exception>

class Logger {
public:
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
    static constexpr int MAX_LOG_LINES = 100;
    
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void initialize(const std::string& logFilePath) {
        this->logFilePath = logFilePath;
        
        // Read existing log file and keep only last MAX_LOG_LINES
        std::vector<std::string> existingLines;
        std::ifstream inFile(logFilePath);
        if (inFile.is_open()) {
            std::string line;
            while (std::getline(inFile, line)) {
                existingLines.push_back(line);
            }
            inFile.close();
        }
        
        // Keep only the last MAX_LOG_LINES
        if (existingLines.size() > MAX_LOG_LINES) {
            existingLines.erase(existingLines.begin(), existingLines.end() - MAX_LOG_LINES);
        }
        
        // Write back the kept lines
        std::ofstream outFile(logFilePath, std::ios::trunc);
        if (outFile.is_open()) {
            for (const auto& line : existingLines) {
                outFile << line << std::endl;
            }
            outFile.close();
        }
        
        // Now open in append mode for new logs
        logFile.open(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            // Write initialization message directly without using log() to avoid recursion
            std::string timestamp = getTimestamp();
            std::string logMessage = "[" + timestamp + "] [INFO] Logger initialized - Log file: " + logFilePath;
            logBuffer.push_back(logMessage);
            if (logBuffer.size() > MAX_LOG_LINES) {
                logBuffer.pop_front();
            }
            logFile << logMessage << std::endl;
            logFile.flush();
        } else {
            // Try to create in current directory as fallback
            std::string fallbackPath = "OpenBongo.log";
            this->logFilePath = fallbackPath;
            logFile.open(fallbackPath, std::ios::app);
            if (logFile.is_open()) {
                std::string timestamp = getTimestamp();
                std::string logMessage = "[" + timestamp + "] [WARNING] Could not open log file at: " + logFilePath + ", using: " + fallbackPath;
                logBuffer.push_back(logMessage);
                if (logBuffer.size() > MAX_LOG_LINES) {
                    logBuffer.pop_front();
                }
                logFile << logMessage << std::endl;
                logFile.flush();
            }
        }
    }
    
    void log(const std::string& message, LogLevel level = LogLevel::INFO) {
        std::string timestamp = getTimestamp();
        std::string levelStr = levelToString(level);
        std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
        
        // Add to rolling buffer
        logBuffer.push_back(logMessage);
        if (logBuffer.size() > MAX_LOG_LINES) {
            logBuffer.pop_front();
        }
        
        // Write to file if open
        if (logFile.is_open()) {
            logFile << logMessage << std::endl;
            logFile.flush();
            
            // Periodically rewrite the entire file to keep it at MAX_LOG_LINES
            static int writeCount = 0;
            writeCount++;
            if (writeCount >= 50) { // Rewrite every 50 log entries
                writeCount = 0;
                rewriteLogFile();
            }
        }
        
        // Also write to console (if visible)
        std::cout << logMessage << std::endl;
    }
    
    ~Logger() {
        if (logFile.is_open()) {
            log("Logger shutting down", LogLevel::INFO);
            rewriteLogFile(); // Final rewrite to ensure file is trimmed
            logFile.close();
        }
    }
    
private:
    
    std::ofstream logFile;
    std::string logFilePath;
    std::deque<std::string> logBuffer; // Rolling buffer of log lines
    
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void rewriteLogFile() {
        if (logFilePath.empty() || logBuffer.empty()) {
            return;
        }
        
        // Close current file
        if (logFile.is_open()) {
            logFile.close();
        }
        
        // Rewrite file with only the last MAX_LOG_LINES
        std::ofstream outFile(logFilePath, std::ios::trunc);
        if (outFile.is_open()) {
            for (const auto& line : logBuffer) {
                outFile << line << std::endl;
            }
            outFile.close();
        }
        
        // Reopen in append mode
        logFile.open(logFilePath, std::ios::app);
    }
    
    std::string getTimestamp() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
    
    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

// Convenience macros - use inline functions to avoid macro issues
inline void LOG_DEBUG(const std::string& msg) { Logger::getInstance().log(msg, Logger::LogLevel::DEBUG); }
inline void LOG_INFO(const std::string& msg) { Logger::getInstance().log(msg, Logger::LogLevel::INFO); }
inline void LOG_WARNING(const std::string& msg) { Logger::getInstance().log(msg, Logger::LogLevel::WARNING); }
inline void LOG_ERROR(const std::string& msg) { Logger::getInstance().log(msg, Logger::LogLevel::ERROR); }


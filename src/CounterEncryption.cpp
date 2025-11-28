#include "CounterEncryption.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

namespace CounterEncryption {
    // Generate a key based on system-specific information
    // This makes it harder to decrypt without the actual system
    static uint32_t generateKey() {
        uint32_t key = 0x4B6F6E67; // "Kong" in hex (obfuscated)
        
        #ifdef _WIN32
        // Mix in Windows-specific values
        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        if (GetComputerNameA(computerName, &size)) {
            for (size_t i = 0; i < size && i < 4; ++i) {
                key ^= (static_cast<uint32_t>(computerName[i]) << (i * 8));
            }
        }
        
        // Mix in username
        char userName[256];
        size = sizeof(userName);
        if (GetUserNameA(userName, &size)) {
            for (size_t i = 0; i < size && i < 4; ++i) {
                key ^= (static_cast<uint32_t>(userName[i]) << ((i % 4) * 8));
            }
        }
        #else
        // Mix in Unix-specific values
        uid_t uid = getuid();
        key ^= static_cast<uint32_t>(uid);
        
        gid_t gid = getgid();
        key ^= (static_cast<uint32_t>(gid) << 16);
        #endif
        
        // Ensure key is never zero (would make encryption ineffective)
        if (key == 0) {
            key = 0xDEADBEEF;
        }
        
        return key;
    }
    
    // Simple XOR encryption with key rotation
    static void xorEncryptDecrypt(std::vector<uint8_t>& data, uint32_t key) {
        uint32_t currentKey = key;
        for (size_t i = 0; i < data.size(); ++i) {
            // XOR with current byte of key
            data[i] ^= static_cast<uint8_t>(currentKey & 0xFF);
            
            // Rotate key for next byte
            currentKey = (currentKey << 1) | (currentKey >> 31);
            
            // Add some additional obfuscation
            data[i] ^= static_cast<uint8_t>((i * 0x9E3779B9) & 0xFF);
        }
    }
    
    std::string encryptCounter(int value) {
        // Convert int to bytes
        std::vector<uint8_t> data(sizeof(int));
        std::memcpy(data.data(), &value, sizeof(int));
        
        // Add a magic header to verify decryption
        std::vector<uint8_t> encrypted;
        encrypted.push_back(0x42); // Magic byte 1
        encrypted.push_back(0x4F); // Magic byte 2 ("BO" for Bongo)
        encrypted.push_back(0x4E); // Magic byte 3
        encrypted.push_back(0x47); // Magic byte 4
        
        // Add version byte
        encrypted.push_back(0x01);
        
        // Encrypt the counter value
        uint32_t key = generateKey();
        xorEncryptDecrypt(data, key);
        
        // Append encrypted data
        encrypted.insert(encrypted.end(), data.begin(), data.end());
        
        // Add checksum (simple sum of encrypted bytes)
        uint8_t checksum = 0;
        for (size_t i = 4; i < encrypted.size(); ++i) {
            checksum ^= encrypted[i];
        }
        encrypted.push_back(checksum);
        
        return std::string(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
    }
    
    int decryptCounter(const std::string& encryptedData) {
        if (encryptedData.size() < 10) { // Minimum size: 4 magic bytes + 1 version + 4 data + 1 checksum
            return 0;
        }
        
        const uint8_t* data = reinterpret_cast<const uint8_t*>(encryptedData.data());
        size_t dataSize = encryptedData.size();
        
        // Verify magic header
        if (data[0] != 0x42 || data[1] != 0x4F || data[2] != 0x4E || data[3] != 0x47) {
            return 0; // Invalid magic header
        }
        
        // Verify checksum
        uint8_t checksum = 0;
        for (size_t i = 4; i < dataSize - 1; ++i) {
            checksum ^= data[i];
        }
        if (checksum != data[dataSize - 1]) {
            return 0; // Checksum mismatch
        }
        
        // Extract encrypted counter bytes
        std::vector<uint8_t> counterBytes(data + 5, data + 5 + sizeof(int));
        
        // Decrypt
        uint32_t key = generateKey();
        xorEncryptDecrypt(counterBytes, key);
        
        // Convert back to int
        int value;
        std::memcpy(&value, counterBytes.data(), sizeof(int));
        
        return value;
    }
    
    bool saveEncryptedCounter(const std::string& filePath, int value) {
        try {
            std::string encrypted = encryptCounter(value);
            std::ofstream outFile(filePath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(encrypted.data(), encrypted.size());
                outFile.close();
                return true;
            }
        } catch (...) {
            // Silently fail - encryption errors shouldn't crash the app
        }
        return false;
    }
    
    int loadEncryptedCounter(const std::string& filePath) {
        try {
            std::ifstream inFile(filePath, std::ios::binary);
            if (!inFile.is_open()) {
                return 0; // File doesn't exist, start at 0
            }
            
            // Read entire file
            std::string encryptedData((std::istreambuf_iterator<char>(inFile)),
                                     std::istreambuf_iterator<char>());
            inFile.close();
            
            if (encryptedData.empty()) {
                return 0;
            }
            
            // Try to decrypt
            int value = decryptCounter(encryptedData);
            
            // If decryption fails, try reading as plain text (for migration from old format)
            if (value == 0 && encryptedData.size() < 20) {
                // Might be old plain text format, try to parse it
                try {
                    value = std::stoi(encryptedData);
                    // If successful, immediately save in new encrypted format
                    saveEncryptedCounter(filePath, value);
                } catch (...) {
                    // Not a valid number, return 0
                }
            }
            
            return value;
        } catch (...) {
            // On any error, return 0
            return 0;
        }
    }
}


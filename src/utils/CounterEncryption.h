#ifndef COUNTER_ENCRYPTION_H
#define COUNTER_ENCRYPTION_H

#include <string>
#include <cstdint>

namespace CounterEncryption {
    // Encrypt a counter value to a binary string
    // Returns encrypted data as a string of bytes
    std::string encryptCounter(int value);
    
    // Decrypt a counter value from encrypted binary string
    // Returns the decrypted counter value, or 0 if decryption fails
    int decryptCounter(const std::string& encryptedData);
    
    // Save encrypted counter to file
    bool saveEncryptedCounter(const std::string& filePath, int value);
    
    // Load encrypted counter from file
    // Returns the counter value, or 0 if file doesn't exist or decryption fails
    int loadEncryptedCounter(const std::string& filePath);
}

#endif // COUNTER_ENCRYPTION_H


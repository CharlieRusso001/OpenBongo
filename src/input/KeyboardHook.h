#pragma once

#include <functional>

class KeyboardHook {
public:
    using KeyPressCallback = std::function<void(unsigned int keyCode, bool isPressed)>;
    
    KeyboardHook();
    ~KeyboardHook();
    
    bool initialize(KeyPressCallback callback);
    void shutdown();
    
    bool isInitialized() const { return initialized; }
    
private:
    bool initialized;
    KeyPressCallback onKeyPress;
    
    // Platform-specific implementation
    void* platformData;
    
    friend class PlatformKeyboardHook;
};


#include "input/KeyboardHook.h"

#ifdef _WIN32
#include "platform/WindowsKeyboardHook.h"
#elif __APPLE__
#include "platform/MacKeyboardHook.h"
#endif

KeyboardHook::KeyboardHook() 
    : initialized(false), platformData(nullptr) {
}

KeyboardHook::~KeyboardHook() {
    shutdown();
}

bool KeyboardHook::initialize(KeyPressCallback callback) {
    if (initialized) {
        return true;
    }
    
#ifdef _WIN32
    platformData = new WindowsKeyboardHook();
    WindowsKeyboardHook* hook = static_cast<WindowsKeyboardHook*>(platformData);
    // Convert KeyboardHook callback to WindowsKeyboardHook callback
    WindowsKeyboardHook::KeyPressCallback winCallback = [callback](DWORD vkCode, bool isPressed) {
        callback(static_cast<unsigned int>(vkCode), isPressed);
    };
    initialized = hook->initialize(winCallback);
#elif __APPLE__
    platformData = new MacKeyboardHook();
    MacKeyboardHook* hook = static_cast<MacKeyboardHook*>(platformData);
    // Mac keyboard hook still uses old callback signature
    MacKeyboardHook::KeyPressCallback macCallback = [callback]() {
        // For Mac, we don't have key code info in the old callback
        // Use a dummy key code (0) - this might need to be updated for Mac
        callback(0, true);
    };
    initialized = hook->initialize(macCallback);
#else
    return false;
#endif
    
    if (initialized) {
        onKeyPress = callback;
    }
    
    return initialized;
}

void KeyboardHook::shutdown() {
    if (!initialized) {
        return;
    }
    
#ifdef _WIN32
    if (platformData) {
        WindowsKeyboardHook* hook = static_cast<WindowsKeyboardHook*>(platformData);
        hook->shutdown();
        delete hook;
        platformData = nullptr;
    }
#elif __APPLE__
    if (platformData) {
        MacKeyboardHook* hook = static_cast<MacKeyboardHook*>(platformData);
        hook->shutdown();
        delete hook;
        platformData = nullptr;
    }
#endif
    
    initialized = false;
    onKeyPress = nullptr;
}


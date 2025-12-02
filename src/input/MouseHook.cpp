#include "input/MouseHook.h"

#ifdef _WIN32
#include "platform/WindowsMouseHook.h"
#elif __APPLE__
// TODO: Add Mac mouse hook if needed
#endif

MouseHook::MouseHook() 
    : initialized(false), platformData(nullptr) {
}

MouseHook::~MouseHook() {
    shutdown();
}

bool MouseHook::initialize(MouseClickCallback callback) {
    if (initialized) {
        return true;
    }
    
#ifdef _WIN32
    platformData = new WindowsMouseHook();
    WindowsMouseHook* hook = static_cast<WindowsMouseHook*>(platformData);
    // Convert MouseHook callback to WindowsMouseHook callback
    WindowsMouseHook::MouseClickCallback winCallback = [callback](WindowsMouseHook::ButtonType button, bool isPressed) {
        // Convert WindowsMouseHook::ButtonType to MouseHook::ButtonType
        MouseHook::ButtonType hookButton = static_cast<MouseHook::ButtonType>(button);
        callback(hookButton, isPressed);
    };
    initialized = hook->initialize(winCallback);
#elif __APPLE__
    // TODO: Add Mac mouse hook implementation
    return false;
#else
    return false;
#endif
    
    if (initialized) {
        onMouseClick = callback;
    }
    
    return initialized;
}

void MouseHook::shutdown() {
    if (!initialized) {
        return;
    }
    
#ifdef _WIN32
    if (platformData) {
        WindowsMouseHook* hook = static_cast<WindowsMouseHook*>(platformData);
        hook->shutdown();
        delete hook;
        platformData = nullptr;
    }
#elif __APPLE__
    // TODO: Add Mac mouse hook cleanup
#endif
    
    initialized = false;
    onMouseClick = nullptr;
}


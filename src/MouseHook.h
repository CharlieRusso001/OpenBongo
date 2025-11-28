#pragma once

#include <functional>

class MouseHook {
public:
    enum ButtonType {
        BUTTON_LEFT = 0,
        BUTTON_RIGHT = 1,
        BUTTON_MIDDLE = 2
    };
    
    using MouseClickCallback = std::function<void(ButtonType button, bool isPressed)>;
    
    MouseHook();
    ~MouseHook();
    
    bool initialize(MouseClickCallback callback);
    void shutdown();
    
    bool isInitialized() const { return initialized; }
    
private:
    bool initialized;
    MouseClickCallback onMouseClick;
    
    // Platform-specific implementation
    void* platformData;
    
    friend class PlatformMouseHook;
};


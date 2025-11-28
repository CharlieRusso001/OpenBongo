#pragma once

#ifdef __APPLE__

#include <functional>
#include <CoreGraphics/CoreGraphics.h>

class MacKeyboardHook {
public:
    using KeyPressCallback = std::function<void()>;
    
    MacKeyboardHook();
    ~MacKeyboardHook();
    
    bool initialize(KeyPressCallback callback);
    void shutdown();
    
    bool isInitialized() const { return eventTap != nullptr; }
    
private:
    static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon);
    
    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
    KeyPressCallback onKeyPress;
};

#endif // __APPLE__


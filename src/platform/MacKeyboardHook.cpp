#ifdef __APPLE__

#include "MacKeyboardHook.h"
#include <iostream>
#include <ApplicationServices/ApplicationServices.h>

MacKeyboardHook::MacKeyboardHook()
    : eventTap(nullptr), runLoopSource(nullptr) {
}

MacKeyboardHook::~MacKeyboardHook() {
    shutdown();
}

bool MacKeyboardHook::initialize(KeyPressCallback callback) {
    if (eventTap != nullptr) {
        return true;
    }
    
    onKeyPress = callback;
    
    // Request accessibility permissions (user needs to grant this)
    CGEventMask eventMask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged);
    
    eventTap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        eventMask,
        eventTapCallback,
        this
    );
    
    if (eventTap == nullptr) {
        std::cerr << "Failed to create event tap. You may need to grant accessibility permissions." << std::endl;
        return false;
    }
    
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    if (runLoopSource == nullptr) {
        std::cerr << "Failed to create run loop source." << std::endl;
        CGEventTapEnable(eventTap, false);
        CFRelease(eventTap);
        eventTap = nullptr;
        return false;
    }
    
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    
    return true;
}

void MacKeyboardHook::shutdown() {
    if (runLoopSource != nullptr) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
        CFRelease(runLoopSource);
        runLoopSource = nullptr;
    }
    
    if (eventTap != nullptr) {
        CGEventTapEnable(eventTap, false);
        CFRelease(eventTap);
        eventTap = nullptr;
    }
    
    onKeyPress = nullptr;
}

CGEventRef MacKeyboardHook::eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon) {
    MacKeyboardHook* hook = static_cast<MacKeyboardHook*>(refcon);
    
    if (type == kCGEventKeyDown) {
        if (hook && hook->onKeyPress) {
            hook->onKeyPress();
        }
    }
    
    return event;
}

#endif // __APPLE__


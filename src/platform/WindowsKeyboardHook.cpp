#ifdef _WIN32

#include "WindowsKeyboardHook.h"
#include <iostream>

WindowsKeyboardHook* WindowsKeyboardHook::instance = nullptr;
WindowsKeyboardHook::KeyPressCallback WindowsKeyboardHook::callback = nullptr;

WindowsKeyboardHook::WindowsKeyboardHook() 
    : hook(nullptr), hookInstalled(false) {
    instance = this;
}

WindowsKeyboardHook::~WindowsKeyboardHook() {
    shutdown();
    if (instance == this) {
        instance = nullptr;
    }
}

bool WindowsKeyboardHook::initialize(KeyPressCallback cb) {
    if (hookInstalled) {
        return true;
    }
    
    callback = cb;
    
    // Install low-level keyboard hook
    hook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandle(nullptr),
        0
    );
    
    if (hook == nullptr) {
        std::cerr << "Failed to install keyboard hook. Error: " << GetLastError() << std::endl;
        return false;
    }
    
    hookInstalled = true;
    return true;
}

void WindowsKeyboardHook::shutdown() {
    if (hook != nullptr) {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
        hookInstalled = false;
    }
    callback = nullptr;
}

LRESULT CALLBACK WindowsKeyboardHook::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = kbdStruct->vkCode;
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Key pressed
            if (callback) {
                callback(vkCode, true);
            }
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            // Key released
            if (callback) {
                callback(vkCode, false);
            }
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

#endif // _WIN32


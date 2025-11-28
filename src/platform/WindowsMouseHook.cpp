#ifdef _WIN32

#include "WindowsMouseHook.h"
#include <iostream>

WindowsMouseHook* WindowsMouseHook::instance = nullptr;
WindowsMouseHook::MouseClickCallback WindowsMouseHook::callback = nullptr;

WindowsMouseHook::WindowsMouseHook() 
    : hook(nullptr), hookInstalled(false) {
    instance = this;
}

WindowsMouseHook::~WindowsMouseHook() {
    shutdown();
    if (instance == this) {
        instance = nullptr;
    }
}

bool WindowsMouseHook::initialize(MouseClickCallback cb) {
    if (hookInstalled) {
        return true;
    }
    
    callback = cb;
    
    // Install low-level mouse hook
    hook = SetWindowsHookEx(
        WH_MOUSE_LL,
        LowLevelMouseProc,
        GetModuleHandle(nullptr),
        0
    );
    
    if (hook == nullptr) {
        std::cerr << "Failed to install mouse hook. Error: " << GetLastError() << std::endl;
        return false;
    }
    
    hookInstalled = true;
    return true;
}

void WindowsMouseHook::shutdown() {
    if (hook != nullptr) {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
        hookInstalled = false;
    }
    callback = nullptr;
}

LRESULT CALLBACK WindowsMouseHook::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_LBUTTONDOWN) {
            // Left mouse button pressed
            if (callback) {
                callback(BUTTON_LEFT, true);
            }
        }
        else if (wParam == WM_LBUTTONUP) {
            // Left mouse button released
            if (callback) {
                callback(BUTTON_LEFT, false);
            }
        }
        else if (wParam == WM_RBUTTONDOWN) {
            // Right mouse button pressed
            if (callback) {
                callback(BUTTON_RIGHT, true);
            }
        }
        else if (wParam == WM_RBUTTONUP) {
            // Right mouse button released
            if (callback) {
                callback(BUTTON_RIGHT, false);
            }
        }
        else if (wParam == WM_MBUTTONDOWN) {
            // Middle mouse button pressed
            if (callback) {
                callback(BUTTON_MIDDLE, true);
            }
        }
        else if (wParam == WM_MBUTTONUP) {
            // Middle mouse button released
            if (callback) {
                callback(BUTTON_MIDDLE, false);
            }
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

#endif // _WIN32


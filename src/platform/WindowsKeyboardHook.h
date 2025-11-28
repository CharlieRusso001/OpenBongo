#pragma once

#ifdef _WIN32

#include <windows.h>
#include <functional>

class WindowsKeyboardHook {
public:
    using KeyPressCallback = std::function<void(DWORD vkCode, bool isPressed)>;
    
    WindowsKeyboardHook();
    ~WindowsKeyboardHook();
    
    bool initialize(KeyPressCallback callback);
    void shutdown();
    
    bool isInitialized() const { return hookInstalled; }
    
private:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static WindowsKeyboardHook* instance;
    static KeyPressCallback callback;
    
    HHOOK hook;
    bool hookInstalled;
};

#endif // _WIN32


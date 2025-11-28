#pragma once

#ifdef _WIN32

#include <windows.h>
#include <functional>

class WindowsMouseHook {
public:
    enum ButtonType {
        BUTTON_LEFT = 0,
        BUTTON_RIGHT = 1,
        BUTTON_MIDDLE = 2
    };
    
    using MouseClickCallback = std::function<void(ButtonType button, bool isPressed)>;
    
    WindowsMouseHook();
    ~WindowsMouseHook();
    
    bool initialize(MouseClickCallback callback);
    void shutdown();
    
    bool isInitialized() const { return hookInstalled; }
    
private:
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
    static WindowsMouseHook* instance;
    static MouseClickCallback callback;
    
    HHOOK hook;
    bool hookInstalled;
};

#endif // _WIN32


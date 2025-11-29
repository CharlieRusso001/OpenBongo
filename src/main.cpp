#include <SFML/Graphics.hpp>
#include <iostream>
#include <variant>
#include <type_traits>
#include <optional>
#include <string>
#include <sstream>
#include <map>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "BongoCat.h"
#include "KeyboardHook.h"
#include "MouseHook.h"
#include "Logger.h"
#include "CatPackConfig.h"
#include "CatPackManager.h"
#include "HatConfig.h"
#include "HatManager.h"
#include "CounterEncryption.h"
#include "BongoStats.h"
#include "WebViewWindow.h"
#include "ImageHelper.h"
#include <sstream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "shell32.lib")

// Global crash handler
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    // Write directly to log file in logs folder (don't use Logger singleton which might be destroyed)
    std::string crashLogPath = "logs/OpenBongo.log";
    #ifdef _WIN32
    // Try to get executable directory
    char exePath[MAX_PATH];
    DWORD result = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    if (result != 0 && result < MAX_PATH) {
        try {
            std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
            crashLogPath = (exeDir / "logs" / "OpenBongo.log").string();
        } catch (...) {
            // Fallback to current directory
            crashLogPath = "logs/OpenBongo.log";
        }
    }
    #endif
    
    // Ensure logs directory exists
    try {
        std::filesystem::path logDir = std::filesystem::path(crashLogPath).parent_path();
        std::filesystem::create_directories(logDir);
    } catch (...) {
        // If we can't create directory, try current directory
        crashLogPath = "OpenBongo.log";
    }
    
    std::ofstream crashLog(crashLogPath, std::ios::app);
    if (crashLog.is_open()) {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        crashLog << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] [CRASH] ";
        crashLog << "Exception Code: 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::dec;
        crashLog << " at address: 0x" << std::hex << (void*)ExceptionInfo->ExceptionRecord->ExceptionAddress << std::dec;
        crashLog << std::endl;
        
        if (ExceptionInfo->ContextRecord) {
            #ifdef _WIN64
            crashLog << "RIP: 0x" << std::hex << ExceptionInfo->ContextRecord->Rip << std::dec << std::endl;
            #else
            crashLog << "EIP: 0x" << std::hex << ExceptionInfo->ContextRecord->Eip << std::dec << std::endl;
            #endif
        }
        crashLog.flush();
        crashLog.close();
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

// Get taskbar position and size
struct TaskbarInfo {
    int x, y, width, height;
    bool isHorizontal; // true if taskbar is at top or bottom
    bool isAtBottom; // true if taskbar is at bottom
};

TaskbarInfo GetTaskbarInfo() {
    TaskbarInfo info = {0, 0, 0, 0, true, true};
    
    APPBARDATA abd = {0};
    abd.cbSize = sizeof(APPBARDATA);
    
    // Get taskbar position
    UINT taskbarPos = SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
    if (taskbarPos) {
        info.x = abd.rc.left;
        info.y = abd.rc.top;
        info.width = abd.rc.right - abd.rc.left;
        info.height = abd.rc.bottom - abd.rc.top;
        
        // Determine if taskbar is horizontal (top or bottom) or vertical (left or right)
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        if (info.width > info.height) {
            // Horizontal taskbar
            info.isHorizontal = true;
            info.isAtBottom = (info.y > screenHeight / 2);
        } else {
            // Vertical taskbar
            info.isHorizontal = false;
            info.isAtBottom = false; // Vertical taskbars don't have a "bottom" concept for snapping
        }
    }
    
    return info;
}
#endif

int main() {
    // Set up crash handler FIRST, before anything else
    #ifdef _WIN32
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
    #endif
    
    // Get executable directory and create organized folder structure
    std::string exeDirPath = ""; // Will store executable directory if available
    std::string logsDirPath = "logs";
    std::string statsDirPath = "stats";
    
    #ifdef _WIN32
    // Try to get executable directory
    char exePath[MAX_PATH];
    DWORD result = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    if (result != 0 && result < MAX_PATH) {
        try {
            std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
            exeDirPath = exeDir.string();
            logsDirPath = (exeDir / "logs").string();
            statsDirPath = (exeDir / "stats").string();
            
            // Create logs and stats directories if they don't exist
            std::filesystem::create_directories(logsDirPath);
            std::filesystem::create_directories(statsDirPath);
        } catch (...) {
            // If filesystem fails, use current directory
            exeDirPath = "";
            logsDirPath = "logs";
            statsDirPath = "stats";
            try {
                std::filesystem::create_directories(logsDirPath);
                std::filesystem::create_directories(statsDirPath);
            } catch (...) {
                // If we can't create directories, use current directory
                logsDirPath = ".";
                statsDirPath = ".";
            }
        }
    } else {
        // Fallback: try to create directories in current directory
        try {
            std::filesystem::create_directories(logsDirPath);
            std::filesystem::create_directories(statsDirPath);
        } catch (...) {
            logsDirPath = ".";
            statsDirPath = ".";
        }
    }
    #else
    // For non-Windows, create directories in current directory
    try {
        std::filesystem::create_directories(logsDirPath);
        std::filesystem::create_directories(statsDirPath);
    } catch (...) {
        logsDirPath = ".";
        statsDirPath = ".";
    }
    #endif
    
    // Initialize logger - save log file in logs folder
    std::string logPath = (std::filesystem::path(logsDirPath) / "OpenBongo.log").string();
    try {
        Logger::getInstance().initialize(logPath);
        LOG_INFO("Application starting - Log file: " + logPath);
    } catch (const std::exception& e) {
        // If logging fails, continue anyway
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
    }
    
    // Initialize stats tracker - save stats file in stats folder
    std::string statsPath = (std::filesystem::path(statsDirPath) / "BongoStats.log").string();
    try {
        BongoStats::getInstance().initialize(statsPath);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize stats tracker: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR("Unknown exception initializing stats tracker");
    }
    
    #ifdef _WIN32
    // DLLs are now in the same directory as the executable, so no need to set DLL directory
    // Windows will automatically find them in the exe directory
    
    // Hide console window
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole != NULL) {
        ShowWindow(hwndConsole, SW_HIDE);
    }
#endif
    
    // Create a small, always-on-top window (increased height for UI elements)
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(200, 260)), "Bongo Cat", 
                           sf::Style::None);
    LOG_INFO("Window created");
    
    // Get window handle early for message posting
    HWND hwnd = nullptr;
#ifdef _WIN32
    hwnd = window.getNativeHandle();
    
    // Make window always on top and transparent (platform-specific)
    
    // Set extended style to include topmost and layered window
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED | WS_EX_TOPMOST;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
    
    // Set window to be always on top (use HWND_TOPMOST with SWP_NOACTIVATE to prevent focus stealing)
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    
    // Use a specific color key (magenta) for transparency - a color we won't use in the UI
    // This makes pixels with this exact color transparent
    COLORREF transparentColor = RGB(255, 0, 255); // Magenta
    SetLayeredWindowAttributes(hwnd, transparentColor, 0, LWA_COLORKEY);
    
    // Add system menu so window can be closed from taskbar
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style |= WS_SYSMENU;
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    
    // Get system menu and add close option
    HMENU hMenu = GetSystemMenu(hwnd, FALSE);
    if (hMenu) {
        // Enable close menu item
        EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
    }
#elif __APPLE__
    // On macOS, SFML handles the window styling
    // You can make it frameless later if needed
#endif
    
    // Scan for available cat packs
    std::vector<CatPackConfig> availableCatPacks = CatPackManager::scanForCatPacks();
    if (availableCatPacks.empty()) {
        // Add default if none found
        availableCatPacks.push_back(CatPackManager::getDefaultCatPack());
    }
    
    // Load selected cat pack (default to first one, or load from file)
    std::string selectedCatPackName = availableCatPacks[0].name;
    std::string catPackConfigPath = "OpenBongo.catpack";
    #ifdef _WIN32
    if (!exeDirPath.empty()) {
        try {
            catPackConfigPath = (std::filesystem::path(exeDirPath) / "OpenBongo.catpack").string();
        } catch (...) {
            catPackConfigPath = "OpenBongo.catpack";
        }
    }
    #endif
    
    // Load saved cat pack selection
    std::ifstream catPackFile(catPackConfigPath);
    if (catPackFile.is_open()) {
        std::getline(catPackFile, selectedCatPackName);
        catPackFile.close();
    }
    
    // Find the selected cat pack
    CatPackConfig currentCatPack = CatPackManager::findCatPackByName(availableCatPacks, selectedCatPackName);
    if (currentCatPack.name != selectedCatPackName) {
        // Selected pack not found, use first available
        currentCatPack = availableCatPacks[0];
        selectedCatPackName = currentCatPack.name;
    }
    
    // Scan for available hats
    std::vector<HatConfig> availableHats = HatManager::scanForHats();
    // Add "No Hat" option at the beginning
    availableHats.insert(availableHats.begin(), HatManager::getNoHat());
    
    // Load selected hat (default to "No Hat", or load from file)
    std::string selectedHatName = "No Hat";
    std::string hatConfigPath = "OpenBongo.hat";
    #ifdef _WIN32
    if (!exeDirPath.empty()) {
        try {
            hatConfigPath = (std::filesystem::path(exeDirPath) / "OpenBongo.hat").string();
        } catch (...) {
            hatConfigPath = "OpenBongo.hat";
        }
    }
    #endif
    
    // Load saved hat selection
    std::ifstream hatFile(hatConfigPath);
    if (hatFile.is_open()) {
        std::getline(hatFile, selectedHatName);
        hatFile.close();
    }
    
    // Find the selected hat
    HatConfig currentHat = HatManager::findHatByName(availableHats, selectedHatName);
    if (currentHat.name != selectedHatName) {
        // Selected hat not found, use "No Hat"
        currentHat = HatManager::getNoHat();
        selectedHatName = currentHat.name;
    }
    
    // Create Bongo Cat with selected pack configuration
    // Load saved cat size or use default
    float catSize = 100.0f;
    std::string catSizeConfigPath = "OpenBongo.catsize";
    #ifdef _WIN32
    if (!exeDirPath.empty()) {
        try {
            catSizeConfigPath = (std::filesystem::path(exeDirPath) / "OpenBongo.catsize").string();
        } catch (...) {
            catSizeConfigPath = "OpenBongo.catsize";
        }
    }
    #endif
    
    // Load saved cat size
    std::ifstream catSizeFile(catSizeConfigPath);
    if (catSizeFile.is_open()) {
        std::string sizeStr;
        std::getline(catSizeFile, sizeStr);
        if (!sizeStr.empty()) {
            try {
                catSize = std::stof(sizeStr);
                if (catSize < 50.0f) catSize = 50.0f;
                if (catSize > 200.0f) catSize = 200.0f;
            } catch (...) {
                catSize = 100.0f;
            }
        }
        catSizeFile.close();
    }
    
    float catX = (200.0f - catSize) / 2.0f;
    float catY = 20.0f; // Position cat near top
    BongoCat bongoCat(catX, catY, catSize, currentCatPack);
    bongoCat.setWindowHeight(260.0f); // Set window height so hands position at bottom
    bongoCat.setHat(currentHat); // Set initial hat
    
    // Position window above taskbar on boot
    #ifdef _WIN32
    TaskbarInfo taskbar = GetTaskbarInfo();
    sf::Vector2u screenSize = sf::VideoMode::getDesktopMode().size;
    int initialX = static_cast<int>(screenSize.x) - 220;
    int initialY;
    
    if (taskbar.isHorizontal && taskbar.isAtBottom && taskbar.height > 0) {
        // Position so cat body bottom aligns with taskbar top
        initialY = taskbar.y - static_cast<int>(bongoCat.getBodyBottomY());
    } else {
        // Fallback to bottom-right corner if taskbar not at bottom
        initialY = static_cast<int>(screenSize.y) - 220;
    }
    
    window.setPosition(sf::Vector2i(initialX, initialY));
    #else
    sf::Vector2u screenSize = sf::VideoMode::getDesktopMode().size;
    window.setPosition(sf::Vector2i(
        static_cast<int>(screenSize.x) - 220,
        static_cast<int>(screenSize.y) - 220
    ));
    #endif
    
    // Counter for clicks and keypresses - load from persistent storage
    int totalCount = 0;
    
    // Load counter from file if it exists - save in stats folder
    std::string counterFilePath = (std::filesystem::path(statsDirPath) / "OpenBongo.counter").string();
    
    // Load counter value from encrypted file
    totalCount = CounterEncryption::loadEncryptedCounter(counterFilePath);
    if (totalCount > 0) {
        LOG_INFO("Counter loaded from encrypted file: " + std::to_string(totalCount));
    } else {
        LOG_INFO("Counter file not found or invalid, starting at 0");
    }
    
    // Function to save counter to encrypted file
    auto saveCounter = [&counterFilePath](int count) {
        if (CounterEncryption::saveEncryptedCounter(counterFilePath, count)) {
        } else {
            LOG_ERROR("Failed to save counter to encrypted file");
        }
    };
    
    // Track key/mouse button states to prevent repeat triggers
    // Use thread-safe approach with simple flags
    std::map<unsigned int, bool> keyStates; // Use virtual key code for tracking
    std::map<int, bool> mouseButtonStates; // Use ButtonType enum value
    
    // Track if taskbar was clicked (for repositioning window)
    bool taskbarWasClicked = false;
    
    // Initialize keyboard hook with counter and state tracking
    KeyboardHook keyboardHook;
    bool keyboardHookInitialized = keyboardHook.initialize([&bongoCat, &totalCount, &keyStates](unsigned int keyCode, bool isPressed) {
        try {
            if (isPressed) {
                // Only trigger if key wasn't already pressed (prevent repeat on hold)
                if (!keyStates[keyCode]) {
                    keyStates[keyCode] = true;
                    totalCount++;
                    BongoStats::getInstance().recordKeyPress(keyCode);
                    bongoCat.punch();
                }
            } else {
                // Key released - reset state
                keyStates[keyCode] = false;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in keyboard hook callback: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in keyboard hook callback");
        }
    });
    
    
    // Initialize mouse hook for global click detection
    MouseHook mouseHook;
    bool mouseHookInitialized = mouseHook.initialize([&bongoCat, &totalCount, &mouseButtonStates, &taskbarWasClicked, &window](MouseHook::ButtonType button, bool isPressed) {
        try {
            std::string buttonName = (button == MouseHook::BUTTON_LEFT) ? "LEFT" : 
                                    (button == MouseHook::BUTTON_RIGHT) ? "RIGHT" : "MIDDLE";
            
            if (isPressed) {
                // Check if click is on taskbar
                #ifdef _WIN32
                POINT cursorPos;
                if (GetCursorPos(&cursorPos)) {
                    TaskbarInfo taskbar = GetTaskbarInfo();
                    if (taskbar.isHorizontal && taskbar.isAtBottom && taskbar.height > 0) {
                        // Check if cursor is within taskbar bounds
                        if (cursorPos.x >= taskbar.x && cursorPos.x <= taskbar.x + taskbar.width &&
                            cursorPos.y >= taskbar.y && cursorPos.y <= taskbar.y + taskbar.height) {
                            taskbarWasClicked = true;
                        } else if (taskbarWasClicked) {
                            // Taskbar was clicked before, now clicking elsewhere - reposition window above taskbar
                            sf::Vector2u screenSize = sf::VideoMode::getDesktopMode().size;
                            int newX = window.getPosition().x;
                            int newY = taskbar.y - static_cast<int>(bongoCat.getBodyBottomY());
                            window.setPosition(sf::Vector2i(newX, newY));
                            taskbarWasClicked = false; // Reset flag
                        }
                    }
                }
                #endif
                
                // Only trigger if button wasn't already pressed (prevent repeat on hold)
                if (!mouseButtonStates[button]) {
                    mouseButtonStates[button] = true;
                    totalCount++;
                    BongoStats::getInstance().recordMouseClick(buttonName);
                    bongoCat.punch();
                }
            } else {
                // Button released - reset state
                mouseButtonStates[button] = false;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in mouse hook callback: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in mouse hook callback");
        }
    });
    
    if (!mouseHookInitialized) {
        LOG_WARNING("Failed to initialize mouse hook. Global clicks may not be detected.");
    } else {
        LOG_INFO("Mouse hook initialized successfully");
    }
    
    if (!keyboardHookInitialized) {
        LOG_WARNING("Failed to initialize keyboard hook. The cat will only react to clicks.");
    } else {
        LOG_INFO("Keyboard hook initialized successfully");
    }
    
    // Load font for counter text
    sf::Font font;
    bool fontLoaded = false;
    
    // Try to load a system font (common Windows font paths)
    #ifdef _WIN32
    const char* fontPaths[] = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/consola.ttf"
    };
    for (const char* path : fontPaths) {
        // SFML 3.0 uses openFromFile
        if (font.openFromFile(path)) {
            fontLoaded = true;
            break;
        }
    }
    #endif
    
    // Create text with font (SFML 3.0 requires font in constructor)
    // Only create if font loaded successfully
    std::unique_ptr<sf::Text> counterTextPtr;
    std::unique_ptr<sf::RectangleShape> counterBoxPtr;
    std::unique_ptr<sf::RectangleShape> menuButtonPtr;
    std::unique_ptr<sf::RectangleShape> menuButtonLine1Ptr;
    std::unique_ptr<sf::RectangleShape> menuButtonLine2Ptr;
    std::unique_ptr<sf::RectangleShape> menuButtonLine3Ptr;
    
    // UI element positions (below the cat) - centered relative to cat
    float uiY = bongoCat.getBodyBottomY() + 10.0f; // Position below cat
    float counterBoxWidth = 84.0f; // 120.0f * 0.7
    float counterBoxHeight = 21.0f; // 30.0f * 0.7
    float menuButtonSize = 21.0f; // 30.0f * 0.7
    float spacing = 10.0f; // Spacing between counter and menu button
    float totalUIWidth = counterBoxWidth + spacing + menuButtonSize;
    float windowWidth = 200.0f;
    // Center the UI elements relative to the window (which centers them relative to the cat)
    float counterBoxX = (windowWidth - totalUIWidth) / 2.0f;
    float menuButtonX = counterBoxX + counterBoxWidth + spacing;
    
    // Counter box (light grey background)
    counterBoxPtr = std::make_unique<sf::RectangleShape>(sf::Vector2f(counterBoxWidth, counterBoxHeight));
    counterBoxPtr->setPosition(sf::Vector2f(counterBoxX, uiY));
    counterBoxPtr->setFillColor(sf::Color(220, 220, 220)); // Light grey
    
    // Menu button (darker grey square)
    menuButtonPtr = std::make_unique<sf::RectangleShape>(sf::Vector2f(menuButtonSize, menuButtonSize));
    menuButtonPtr->setPosition(sf::Vector2f(menuButtonX, uiY));
    menuButtonPtr->setFillColor(sf::Color(180, 180, 180)); // Darker grey
    
    // Hamburger menu lines (three horizontal lines)
    float lineWidth = 12.6f; // 18.0f * 0.7
    float lineHeight = 1.4f; // 2.0f * 0.7
    float lineSpacing = 2.8f; // 4.0f * 0.7
    float firstLineY = uiY + (menuButtonSize - (lineHeight * 3 + lineSpacing * 2)) / 2.0f;
    
    menuButtonLine1Ptr = std::make_unique<sf::RectangleShape>(sf::Vector2f(lineWidth, lineHeight));
    menuButtonLine1Ptr->setPosition(sf::Vector2f(menuButtonX + (menuButtonSize - lineWidth) / 2.0f, firstLineY));
    menuButtonLine1Ptr->setFillColor(sf::Color(100, 100, 100)); // Dark grey
    
    menuButtonLine2Ptr = std::make_unique<sf::RectangleShape>(sf::Vector2f(lineWidth, lineHeight));
    menuButtonLine2Ptr->setPosition(sf::Vector2f(menuButtonX + (menuButtonSize - lineWidth) / 2.0f, firstLineY + lineHeight + lineSpacing));
    menuButtonLine2Ptr->setFillColor(sf::Color(100, 100, 100)); // Dark grey
    
    menuButtonLine3Ptr = std::make_unique<sf::RectangleShape>(sf::Vector2f(lineWidth, lineHeight));
    menuButtonLine3Ptr->setPosition(sf::Vector2f(menuButtonX + (menuButtonSize - lineWidth) / 2.0f, firstLineY + (lineHeight + lineSpacing) * 2));
    menuButtonLine3Ptr->setFillColor(sf::Color(100, 100, 100)); // Dark grey
    
    // Settings window using WebViewWindow (Crow + HTML UI)
    std::unique_ptr<WebViewWindow> settingsWebView;
    bool settingsWindowOpen = false;
    
    // Helper function to send JSON message to webview
    auto sendJSONToWebView = [](WebViewWindow& webView, const std::string& type, const std::string& jsonData) {
        // Create properly formatted JSON message
        std::string message = "{\"type\":\"" + type + "\",\"data\":" + jsonData + "}";
        webView.postMessage(message);
    };
    
    // Helper function to escape JSON string
    auto escapeJSON = [](const std::string& str) -> std::string {
        std::string escaped;
        for (char c : str) {
            if (c == '\\') escaped += "\\\\";
            else if (c == '"') escaped += "\\\"";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\t') escaped += "\\t";
            else escaped += c;
        }
        return escaped;
    };
    
    // Helper function to convert filesystem path to HTTP URL
    auto pathToUrl = [](const std::string& fsPath, int port) -> std::string {
        if (fsPath.empty()) return "";
        
        try {
            std::string pathStr = fsPath;
            // Normalize path separators first
            std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
            
            // Find catpacks or hats in the path (case-insensitive search)
            std::string pathLower = pathStr;
            std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), ::tolower);
            
            size_t catpacksPos = pathLower.find("catpacks/");
            size_t hatsPos = pathLower.find("hats/");
            
            std::string relativePath;
            if (catpacksPos != std::string::npos) {
                // Extract from "catpacks/" onwards
                relativePath = pathStr.substr(catpacksPos);
            } else if (hatsPos != std::string::npos) {
                // Extract from "hats/" onwards
                relativePath = pathStr.substr(hatsPos);
            } else {
                // Try to get relative path from current directory
                try {
                    std::filesystem::path fullPath(fsPath);
                    std::filesystem::path currentDir = std::filesystem::current_path();
                    std::filesystem::path relative = std::filesystem::relative(fullPath, currentDir);
                    relativePath = relative.string();
                    std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
                } catch (...) {
                    // Last resort: use just the filename
                    std::filesystem::path fullPath(fsPath);
                    relativePath = fullPath.filename().string();
                }
            }
            
            // Ensure path starts with / for URL
            if (relativePath[0] != '/') {
                relativePath = "/" + relativePath;
            }
            
            return "http://localhost:" + std::to_string(port) + relativePath;
        } catch (...) {
            // Fallback: just use the filename
            try {
                std::filesystem::path fullPath(fsPath);
                std::string filename = fullPath.filename().string();
                return "http://localhost:" + std::to_string(port) + "/" + filename;
            } catch (...) {
                return "";
            }
        }
    };
    
    // Helper function to send cat pack list
    auto sendCatPackList = [&sendJSONToWebView, &escapeJSON, &pathToUrl](WebViewWindow& webView, const std::vector<CatPackConfig>& packs) {
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < packs.size(); i++) {
            if (i > 0) ss << ",";
            std::string iconPath = packs[i].getImagePath(packs[i].iconImage);
            std::string iconUrl = iconPath.empty() ? "" : pathToUrl(iconPath, 18080);
            ss << "{\"name\":\"" << escapeJSON(packs[i].name) << "\",\"iconPath\":\"" << escapeJSON(iconUrl) << "\"}";
        }
        ss << "]";
        LOG_INFO("Sending cat pack list: " + ss.str());
        sendJSONToWebView(webView, "catPackList", ss.str());
    };
    
    // Helper function to send hat list
    auto sendHatList = [&sendJSONToWebView, &escapeJSON, &pathToUrl](WebViewWindow& webView, const std::vector<HatConfig>& hats) {
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < hats.size(); i++) {
            if (i > 0) ss << ",";
            std::string iconPath = hats[i].iconImage.empty() ? "" : hats[i].getImagePath(hats[i].iconImage);
            std::string iconUrl = iconPath.empty() ? "" : pathToUrl(iconPath, 18080);
            ss << "{\"name\":\"" << escapeJSON(hats[i].name) << "\",\"iconPath\":\"" << escapeJSON(iconUrl) << "\"}";
        }
        ss << "]";
        LOG_INFO("Sending hat list: " + ss.str());
        sendJSONToWebView(webView, "hatList", ss.str());
    };
    
    // Helper function to send selected cat pack
    auto sendSelectedCatPack = [&sendJSONToWebView, &escapeJSON](WebViewWindow& webView, const CatPackConfig& pack) {
        std::string json = "{\"name\":\"" + escapeJSON(pack.name) + "\"}";
        LOG_INFO("Sending selected cat pack: " + json);
        sendJSONToWebView(webView, "selectedCatPack", json);
    };
    
    // Helper function to send selected hat
    auto sendSelectedHat = [&sendJSONToWebView, &escapeJSON](WebViewWindow& webView, const HatConfig& hat) {
        std::string json = "{\"name\":\"" + escapeJSON(hat.name) + "\"}";
        LOG_INFO("Sending selected hat: " + json);
        sendJSONToWebView(webView, "selectedHat", json);
    };
    
    // Helper function to parse JSON message from webview
    auto parseMessage = [](const std::string& message) -> std::map<std::string, std::string> {
        std::map<std::string, std::string> result;
        // Simple JSON parsing for our use case
        std::regex typeRegex("\"type\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch typeMatch;
        if (std::regex_search(message, typeMatch, typeRegex)) {
            result["type"] = typeMatch[1].str();
        }
        
        // Extract name from data if present
        std::regex nameRegex("\"name\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch nameMatch;
        if (std::regex_search(message, nameMatch, nameRegex)) {
            result["name"] = nameMatch[1].str();
        }
        
        return result;
    };
    
    if (fontLoaded) {
        counterTextPtr = std::make_unique<sf::Text>(font, "0");
        counterTextPtr->setCharacterSize(13); // 18 * 0.7 ≈ 12.6, rounded to 13 for readability
        counterTextPtr->setFillColor(sf::Color(80, 80, 80)); // Dark grey text
        // Center text in counter box (estimate width based on character count)
        // Approximate: character width is about 60% of character size for monospace-like fonts
        std::string initialText = std::to_string(totalCount);
        float estimatedWidth = initialText.length() * counterTextPtr->getCharacterSize() * 0.6f;
        float textX = counterBoxX + (counterBoxWidth - estimatedWidth) / 2.0f;
        float textY = uiY + (counterBoxHeight - counterTextPtr->getCharacterSize()) / 2.0f;
        counterTextPtr->setPosition(sf::Vector2f(textX, textY));
        counterTextPtr->setString(initialText);
    }
    
    // Main loop
    sf::Clock clock;
    bool dragging = false;
    sf::Vector2i dragOffset;
    
    LOG_INFO("Entering main loop");
    int loopIteration = 0;
    
    while (window.isOpen()) {
        loopIteration++;
        
        float deltaTime = clock.restart().asSeconds();
        
        // SFML 3.0 event handling - pollEvent returns optional
        try {
            while (const std::optional<sf::Event> eventOpt = window.pollEvent()) {
                // Get the event value from optional
                const sf::Event& event = *eventOpt;
                
                // SFML 3.0 event handling - check event type using if-else chain
                // Try to access event data based on what type it might be
                if (event.is<sf::Event::Closed>()) {
                    LOG_INFO("Window close event received from SFML pollEvent");
                    try {
                        // Close the window properly
                        window.close();
                        LOG_INFO("Breaking from event loop due to close event");
                        break;
                    } catch (const std::exception& e) {
                        LOG_ERROR("Exception handling close event: " + std::string(e.what()));
                    } catch (...) {
                        LOG_ERROR("Unknown exception handling close event");
                    }
                }
                else if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    // Only handle Escape key for closing window
                    // All other keys are handled by the global keyboard hook
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                        LOG_INFO("Escape key pressed - closing window");
                    window.close();
                }
            }
                else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
                    // Only handle dragging on the window itself (not global clicks)
                if (mousePressed->button == sf::Mouse::Button::Left) {
                        sf::Vector2f mousePos(mousePressed->position.x, mousePressed->position.y);
                        
                        // Check if menu button was clicked
                        if (menuButtonPtr && 
                            mousePos.x >= menuButtonX && mousePos.x <= menuButtonX + menuButtonSize &&
                            mousePos.y >= uiY && mousePos.y <= uiY + menuButtonSize) {
                            // Toggle settings window (WebViewWindow with HTML UI)
                            if (!settingsWindowOpen || !settingsWebView || !settingsWebView->isWindowValid()) {
                                // Create WebViewWindow for settings if it doesn't exist
                                if (!settingsWebView) {
                                    std::string htmlPath = "ui/index.html";
                                    #ifdef _WIN32
                                    if (!exeDirPath.empty()) {
                                        try {
                                            htmlPath = (std::filesystem::path(exeDirPath) / "ui" / "index.html").string();
                                            if (!std::filesystem::exists(htmlPath)) {
                                                htmlPath = "ui/index.html";
                                            }
                                        } catch (...) {
                                            htmlPath = "ui/index.html";
                                        }
                                    }
                                    #endif
                                    
                                    settingsWebView = std::make_unique<WebViewWindow>();
                                    
                                    // Set up message handler
                                    settingsWebView->setMessageHandler([&](const std::string& message) {
                                    try {
                                        LOG_INFO("Received message from webview: " + message);
                                        auto parsed = parseMessage(message);
                                        std::string type = parsed["type"];
                                        LOG_INFO("Parsed message type: " + type);
                                        
                                        if (type == "getCatPacks") {
                                            LOG_INFO("Sending cat packs list");
                                            sendCatPackList(*settingsWebView, availableCatPacks);
                                        } else if (type == "getHats") {
                                            LOG_INFO("Sending hats list");
                                            sendHatList(*settingsWebView, availableHats);
                                        } else if (type == "getSelectedCatPack") {
                                            LOG_INFO("Sending selected cat pack");
                                            sendSelectedCatPack(*settingsWebView, currentCatPack);
                                        } else if (type == "getSelectedHat") {
                                            LOG_INFO("Sending selected hat");
                                            sendSelectedHat(*settingsWebView, currentHat);
                                        } else if (type == "selectCatPack") {
                                            std::string packName = parsed["name"];
                                            CatPackConfig newCatPack = CatPackManager::findCatPackByName(availableCatPacks, packName);
                                            if (newCatPack.name == packName) {
                                                selectedCatPackName = packName;
                                                currentCatPack = newCatPack;
                                                bongoCat.setConfig(newCatPack);
                                                
                                                // Save selection
                                                std::ofstream catPackOutFile(catPackConfigPath);
                                                if (catPackOutFile.is_open()) {
                                                    catPackOutFile << selectedCatPackName;
                                                    catPackOutFile.close();
                                                    LOG_INFO("Cat pack selection saved: " + selectedCatPackName);
                                                }
                                                
                                                sendSelectedCatPack(*settingsWebView, currentCatPack);
                                            }
                                        } else if (type == "selectHat") {
                                            std::string hatName = parsed["name"];
                                            HatConfig newHat = HatManager::findHatByName(availableHats, hatName);
                                            if (newHat.name == hatName) {
                                                selectedHatName = hatName;
                                                currentHat = newHat;
                                                bongoCat.setHat(newHat);
                                                
                                                // Save selection
                                                std::ofstream hatOutFile(hatConfigPath);
                                                if (hatOutFile.is_open()) {
                                                    hatOutFile << selectedHatName;
                                                    hatOutFile.close();
                                                    LOG_INFO("Hat selection saved: " + selectedHatName);
                                                }
                                                
                                                sendSelectedHat(*settingsWebView, currentHat);
                                            }
                                        } else if (type == "setCatSize") {
                                            // Extract size from message
                                            std::regex sizeRegex("\"size\"\\s*:\\s*(\\d+)");
                                            std::smatch sizeMatch;
                                            if (std::regex_search(message, sizeMatch, sizeRegex)) {
                                                try {
                                                    float newSize = std::stof(sizeMatch[1].str());
                                                    if (newSize >= 50.0f && newSize <= 200.0f) {
                                                        // Get current bottom Y position in screen coordinates before changing size
                                                        float oldBodyBottomY = bongoCat.getBodyBottomY();
                                                        sf::Vector2i currentWindowPos = window.getPosition();
                                                        float oldBottomScreenY = currentWindowPos.y + oldBodyBottomY;
                                                        
                                                        // Change cat size
                                                        catSize = newSize;
                                                        bongoCat.setSize(catSize);
                                                        
                                                        // Recalculate cat position to center it horizontally, keep Y at 20.0f
                                                        float catX = (200.0f - catSize) / 2.0f;
                                                        bongoCat.setPosition(catX, 20.0f);
                                                        
                                                        // Adjust window position to keep bottom center fixed
                                                        float newBodyBottomY = bongoCat.getBodyBottomY();
                                                        float bottomYDelta = newBodyBottomY - oldBodyBottomY;
                                                        
                                                        #ifdef _WIN32
                                                        // Check if we should snap to taskbar or preserve position
                                                        TaskbarInfo taskbar = GetTaskbarInfo();
                                                        int newWindowY;
                                                        
                                                        if (taskbar.isHorizontal && taskbar.isAtBottom && taskbar.height > 0) {
                                                            // Snap to taskbar: align cat body bottom with taskbar top
                                                            newWindowY = taskbar.y - static_cast<int>(newBodyBottomY);
                                                        } else {
                                                            // Preserve bottom screen position: adjust window Y by the difference
                                                            newWindowY = currentWindowPos.y - static_cast<int>(bottomYDelta);
                                                        }
                                                        
                                                        window.setPosition(sf::Vector2i(currentWindowPos.x, newWindowY));
                                                        #else
                                                        // On non-Windows, preserve bottom screen position
                                                        int newWindowY = currentWindowPos.y - static_cast<int>(bottomYDelta);
                                                        window.setPosition(sf::Vector2i(currentWindowPos.x, newWindowY));
                                                        #endif
                                                        
                                                        // Save size
                                                        std::ofstream catSizeOutFile(catSizeConfigPath);
                                                        if (catSizeOutFile.is_open()) {
                                                            catSizeOutFile << catSize;
                                                            catSizeOutFile.close();
                                                            LOG_INFO("Cat size saved: " + std::to_string(catSize));
                                                        }
                                                    }
                                                } catch (...) {
                                                    LOG_ERROR("Failed to parse cat size");
                                                }
                                            }
                                        } else if (type == "setAccentColor") {
                                            // Extract color from message
                                            std::regex colorRegex("\"color\"\\s*:\\s*\"([^\"]+)\"");
                                            std::smatch colorMatch;
                                            if (std::regex_search(message, colorMatch, colorRegex)) {
                                                std::string color = colorMatch[1].str();
                                                LOG_INFO("Accent color changed to: " + color);
                                                
                                                // Save accent color to file
                                                std::string accentColorConfigPath = "OpenBongo.accentcolor";
                                                #ifdef _WIN32
                                                if (!exeDirPath.empty()) {
                                                    try {
                                                        accentColorConfigPath = (std::filesystem::path(exeDirPath) / "OpenBongo.accentcolor").string();
                                                    } catch (...) {
                                                        accentColorConfigPath = "OpenBongo.accentcolor";
                                                    }
                                                }
                                                #endif
                                                
                                                std::ofstream accentColorOutFile(accentColorConfigPath);
                                                if (accentColorOutFile.is_open()) {
                                                    accentColorOutFile << color;
                                                    accentColorOutFile.close();
                                                    LOG_INFO("Accent color saved: " + color);
                                                }
                                            }
                                        } else if (type == "hideWindow") {
                                            // Hide the window instead of closing it
                                            if (settingsWebView && settingsWebView->isWindowValid()) {
                                                settingsWebView->hideWindow();
                                                settingsWindowOpen = false;
                                                LOG_INFO("Settings window hidden");
                                            }
                                        } else if (type == "openURL") {
                                            // Extract URL from message
                                            std::regex urlRegex("\"url\"\\s*:\\s*\"([^\"]+)\"");
                                            std::smatch urlMatch;
                                            if (std::regex_search(message, urlMatch, urlRegex)) {
                                                std::string url = urlMatch[1].str();
                                                LOG_INFO("Opening URL in default browser: " + url);
                                                
                                                #ifdef _WIN32
                                                ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                                                #elif __APPLE__
                                                std::string command = "open \"" + url + "\"";
                                                system(command.c_str());
                                                #else
                                                // Linux
                                                std::string command = "xdg-open \"" + url + "\"";
                                                system(command.c_str());
                                                #endif
                                            }
                                        }
                                    } catch (const std::exception& e) {
                                        LOG_ERROR("Exception handling webview message: " + std::string(e.what()));
                                    }
                                    });
                                    
                                    // Initialize webview
                                    if (!settingsWebView->initialize(nullptr, htmlPath)) {
                                        LOG_ERROR("Failed to initialize settings WebView window");
                                        settingsWebView.reset();
                                        continue;
                                    }
                                    
                                    #ifdef _WIN32
                                    HWND settingsHwnd = static_cast<HWND>(settingsWebView->getHwnd());
                                    if (settingsHwnd) {
                                        // Remove standard close button
                                        LONG_PTR style = GetWindowLongPtr(settingsHwnd, GWL_STYLE);
                                        style &= ~WS_SYSMENU; // Remove system menu (which includes close button)
                                        SetWindowLongPtr(settingsHwnd, GWL_STYLE, style);
                                        
                                        SetWindowPos(settingsHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                                    }
                                    #endif
                                    
                                    // Wait a bit for webview to fully load, then send initial data
                                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                    
                                    // Send initial data
                                    LOG_INFO("Sending initial data to webview");
                                    sendCatPackList(*settingsWebView, availableCatPacks);
                                    sendHatList(*settingsWebView, availableHats);
                                    sendSelectedCatPack(*settingsWebView, currentCatPack);
                                    sendSelectedHat(*settingsWebView, currentHat);
                                    
                                    // Send initial cat size
                                    std::string catSizeJson = "{\"size\":" + std::to_string(static_cast<int>(catSize)) + "}";
                                    sendJSONToWebView(*settingsWebView, "catSize", catSizeJson);
                                    
                                    // Load and send initial accent color
                                    std::string accentColorConfigPath = "OpenBongo.accentcolor";
                                    #ifdef _WIN32
                                    if (!exeDirPath.empty()) {
                                        try {
                                            accentColorConfigPath = (std::filesystem::path(exeDirPath) / "OpenBongo.accentcolor").string();
                                        } catch (...) {
                                            accentColorConfigPath = "OpenBongo.accentcolor";
                                        }
                                    }
                                    #endif
                                    
                                    std::string accentColor = "#4a90e2"; // Default blue
                                    std::ifstream accentColorFile(accentColorConfigPath);
                                    if (accentColorFile.is_open()) {
                                        std::string colorStr;
                                        std::getline(accentColorFile, colorStr);
                                        if (!colorStr.empty() && colorStr[0] == '#') {
                                            accentColor = colorStr;
                                        }
                                        accentColorFile.close();
                                    }
                                    
                                    std::string accentColorJson = "{\"color\":\"" + escapeJSON(accentColor) + "\"}";
                                    sendJSONToWebView(*settingsWebView, "accentColor", accentColorJson);
                                }
                                
                                // Show window
                                settingsWebView->showWindow();
                                settingsWindowOpen = true;
                                LOG_INFO("Settings WebView window shown");
                            } else {
                                // Hide settings window instead of closing
                                if (settingsWebView) {
                                    if (settingsWebView->isWindowValid()) {
                                        settingsWebView->hideWindow();
                                        settingsWindowOpen = false;
                                        LOG_INFO("Settings window hidden");
                                    } else {
                                        // Window was destroyed somehow, clean up
                                        settingsWebView.reset();
                                        settingsWindowOpen = false;
                                    }
                                } else {
                                    settingsWindowOpen = false;
                                }
                            }
                        } else {
                            // Start dragging
                            dragging = true;
                            dragOffset = sf::Vector2i(
                                mousePressed->position.x,
                                mousePressed->position.y
                            );
                        }
                }
                else if (mousePressed->button == sf::Mouse::Button::Right) {
                        // Right-click on window itself - close window
                        LOG_INFO("Right-click detected on window - closing window");
                    window.close();
                }
            }
                else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseReleased->button == sf::Mouse::Button::Left) {
                    dragging = false;
                }
            }
                else if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
                if (dragging) {
                        #ifdef _WIN32
                        // Calculate new window position
                        int newX = mouseMoved->position.x + window.getPosition().x - dragOffset.x;
                        int newY = mouseMoved->position.y + window.getPosition().y - dragOffset.y;
                        
                        // Get taskbar info
                        TaskbarInfo taskbar = GetTaskbarInfo();
                        
                        // Always keep window above the taskbar (only for bottom taskbar)
                        if (taskbar.isHorizontal && taskbar.isAtBottom && taskbar.height > 0) {
                            // Get the cat body's bottom Y position in screen coordinates
                            // catY is relative to window, so add window Y position
                            float catBodyBottomScreenY = newY + bongoCat.getBodyBottomY();
                            
                            // Always prevent window from going below taskbar - snap if at or below taskbar top
                            if (catBodyBottomScreenY >= taskbar.y) {
                                // Calculate new window Y so that cat body bottom aligns with taskbar top
                                newY = taskbar.y - static_cast<int>(bongoCat.getBodyBottomY());
                            }
                        }
                        
                        window.setPosition(sf::Vector2i(newX, newY));
                        #else
                    window.setPosition(sf::Vector2i(
                        mouseMoved->position.x + window.getPosition().x - dragOffset.x,
                        mouseMoved->position.y + window.getPosition().y - dragOffset.y
                    ));
                        #endif
                }
            }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in event polling: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in event polling");
        }
        
        // Update
        try {
        bongoCat.update(deltaTime);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in bongoCat.update(): " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in bongoCat.update()");
        }
        
        // Periodically ensure window stays always on top (every frame to prevent taskbar from covering it)
        // Also check for Windows close messages from taskbar
        #ifdef _WIN32
        if (hwnd) {
            // Re-apply topmost status every frame to ensure it stays on top even when taskbar is clicked
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            
            // Process Windows messages to handle close from taskbar
            MSG msg;
            while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_SYSCOMMAND && msg.wParam == SC_CLOSE) {
                    // Close requested from system menu (taskbar)
                    LOG_INFO("Close requested from taskbar/system menu");
                    window.close();
                    break;
                } else if (msg.message == WM_CLOSE) {
                    // Close message received
                    LOG_INFO("WM_CLOSE message received");
                    window.close();
                    break;
                } else {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        #endif
        
        // Update counter text with space formatting (e.g., "4 309")
        try {
            if (counterTextPtr) {
                std::stringstream ss;
                std::string countStr = std::to_string(totalCount);
                
                // Add spaces every 3 digits from right (e.g., "4309" -> "4 309")
                std::string formattedStr;
                int len = countStr.length();
                for (int i = 0; i < len; i++) {
                    if (i > 0 && (len - i) % 3 == 0) {
                        formattedStr += " ";
                    }
                    formattedStr += countStr[i];
                }
                
                counterTextPtr->setString(formattedStr);
                
                // Re-center text in counter box
                if (counterBoxPtr) {
                    // Estimate width based on character count (approximate: 60% of character size per char)
                    float estimatedWidth = formattedStr.length() * counterTextPtr->getCharacterSize() * 0.6f;
                    float textX = counterBoxX + (counterBoxWidth - estimatedWidth) / 2.0f;
                    float textY = uiY + (counterBoxHeight - counterTextPtr->getCharacterSize()) / 2.0f;
                    counterTextPtr->setPosition(sf::Vector2f(textX, textY));
                }
                
                // Save counter and stats periodically (every 100 iterations to avoid too frequent file writes)
                if (loopIteration % 100 == 0) {
                    saveCounter(totalCount);
                    BongoStats::getInstance().saveStats();
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception updating counter text: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception updating counter text");
        }
        
        // Handle settings webview window (messages are handled via the message handler callback)
        // Note: Window can no longer be closed externally - only hidden via custom navbar
        
        // Draw
        try {
            // Use magenta as the transparent color key (matches Windows transparency setting)
            window.clear(sf::Color(255, 0, 255)); // Magenta - will be transparent
            
            // Draw counter box (drawn first so cat appears on top)
            if (counterBoxPtr) {
                window.draw(*counterBoxPtr);
            }
            
            // Draw counter text (drawn first so cat appears on top)
            if (counterTextPtr) {
                window.draw(*counterTextPtr);
            }
            
            // Draw menu button (drawn first so cat appears on top)
            if (menuButtonPtr) {
                window.draw(*menuButtonPtr);
            }
            
            // Draw hamburger menu lines (drawn first so cat appears on top)
            if (menuButtonLine1Ptr) {
                window.draw(*menuButtonLine1Ptr);
            }
            if (menuButtonLine2Ptr) {
                window.draw(*menuButtonLine2Ptr);
            }
            if (menuButtonLine3Ptr) {
                window.draw(*menuButtonLine3Ptr);
            }
            
            // Draw bongo cat (drawn last so it appears above UI elements)
            bongoCat.draw(window);
            
            window.display();
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in draw operations: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in draw operations");
        }
        
        // Settings webview window is rendered by webview itself, no need to draw here
        
        // Log every 1000 iterations for debugging
        if (loopIteration % 1000 == 0) {
        }
    }
    
    LOG_INFO("Exited main loop - Total iterations: " + std::to_string(loopIteration));
    
    LOG_INFO("Application shutting down - Total count: " + std::to_string(totalCount));
    
    // Shutdown hooks BEFORE destroying window
    try {
        LOG_INFO("Shutting down keyboard hook");
    keyboardHook.shutdown();
        LOG_INFO("Keyboard hook shut down successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Exception shutting down keyboard hook: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR("Unknown exception shutting down keyboard hook");
    }
    
    try {
        LOG_INFO("Shutting down mouse hook");
        mouseHook.shutdown();
        LOG_INFO("Mouse hook shut down successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Exception shutting down mouse hook: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR("Unknown exception shutting down mouse hook");
    }
    
    // Close settings webview window if open
    if (settingsWebView) {
        try {
            settingsWebView->shutdown();
            settingsWebView.reset();
            LOG_INFO("Settings webview window closed during shutdown");
        } catch (...) {
            // Ignore errors during shutdown
        }
    }
    
    // Save counter one final time before exit
    try {
            saveCounter(totalCount);
            LOG_INFO("Counter saved on exit: " + std::to_string(totalCount));
            BongoStats::getInstance().saveStats();
    } catch (...) {
        LOG_ERROR("Failed to save counter on exit");
    }
    
    // Explicitly close and destroy window before exit
    try {
        LOG_INFO("Destroying SFML window");
        if (window.isOpen()) {
            window.close();
            LOG_INFO("Window closed");
        }
        // Give SFML time to clean up
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        LOG_INFO("Window cleanup complete");
    } catch (const std::exception& e) {
        LOG_ERROR("Exception destroying window: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR("Unknown exception destroying window");
    }
    
    LOG_INFO("Application shutdown complete - About to return from main()");
    
    // Flush log one more time before exit
    try {
        Logger::getInstance().log("Returning from main()", Logger::LogLevel::INFO);
    } catch (...) {
        // Ignore if logger is already destroyed
    }
    
    return 0;
}


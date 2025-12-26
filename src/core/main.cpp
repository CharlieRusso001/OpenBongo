#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
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
#include <vector>
#include <mutex>
#include <iomanip>
#include <ctime>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "core/BongoCat.h"
#include "input/KeyboardHook.h"
#include "input/MouseHook.h"
#include "utils/Logger.h"
#include "config/CatPackConfig.h"
#include "managers/CatPackManager.h"
#include "config/HatConfig.h"
#include "managers/HatManager.h"
#include "audio/BonkPackConfig.h"
#include "audio/BonkPackManager.h"
#include "utils/CounterEncryption.h"
#include "core/BongoStats.h"
#include "ui/WebViewWindow.h"
#include "utils/ImageHelper.h"
#include <sstream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#include <mmsystem.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winmm.lib")

// Helper function to get AppData folder path
std::string GetAppDataFolder() {
    #ifdef _WIN32
    char appDataPath[MAX_PATH];
    if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appDataPath) == S_OK) {
        try {
            std::filesystem::path appDataDir = std::filesystem::path(appDataPath) / "OpenBongo";
            std::filesystem::create_directories(appDataDir);
            return appDataDir.string();
        } catch (...) {
            // Fallback to current directory if AppData access fails
            return ".";
        }
    }
    #endif
    // Fallback for non-Windows or if AppData access fails
    return ".";
}

// Global crash handler
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
    // Write directly to log file in AppData folder (don't use Logger singleton which might be destroyed)
    std::string crashLogPath = "OpenBongo.log";
    #ifdef _WIN32
        try {
        std::string appDataDir = GetAppDataFolder();
        crashLogPath = (std::filesystem::path(appDataDir) / "OpenBongo.log").string();
        } catch (...) {
            // Fallback to current directory
        crashLogPath = "OpenBongo.log";
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

// Helper to get the work area of the monitor containing a specific point or window
// If hwnd is provided, it takes precedence. If point is provided, it uses that.
RECT GetMonitorWorkArea(HWND hwnd, POINT* point = nullptr) {
    HMONITOR hMonitor = nullptr;
    if (hwnd) {
        hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    } else if (point) {
        hMonitor = MonitorFromPoint(*point, MONITOR_DEFAULTTONEAREST);
    } else {
        // Fallback to primary
        POINT zero = {0, 0};
        hMonitor = MonitorFromPoint(zero, MONITOR_DEFAULTTOPRIMARY);
    }
    
    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (GetMonitorInfo(hMonitor, &mi)) {
        return mi.rcWork;
    }
    
    // Fallback if GetMonitorInfo fails (return full screen of primary)
    RECT screen = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    return screen;
}

// Structure to hold both buffer and sound together (buffer must outlive sound)
struct SoundHolder {
    sf::SoundBuffer buffer;
    sf::Sound sound;
    
    SoundHolder() : sound(buffer) {}
};

// Global vector to keep sounds alive until they finish playing
static std::vector<std::unique_ptr<SoundHolder>> g_activeSounds;
static std::mutex g_soundsMutex;

// Helper function to play sound file using SFML Audio (supports MP3)
void PlaySoundFile(const std::string& soundPath, float volume = 100.0f) {
    if (soundPath.empty()) {
        return; // No sound to play
    }
    
    // Check if file exists
    if (!std::filesystem::exists(soundPath)) {
        LOG_WARNING("Sound file not found: " + soundPath);
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(g_soundsMutex);
        
        // Clean up finished sounds
        g_activeSounds.erase(
            std::remove_if(g_activeSounds.begin(), g_activeSounds.end(),
                [](const std::unique_ptr<SoundHolder>& holder) {
                    return holder->sound.getStatus() == sf::Sound::Status::Stopped;
                }),
            g_activeSounds.end()
        );
        
        // Create sound holder (buffer and sound together)
        auto holder = std::make_unique<SoundHolder>();
        
        // Load sound buffer from file
        if (!holder->buffer.loadFromFile(soundPath)) {
            LOG_WARNING("Failed to load sound file: " + soundPath);
            return;
        }
        
        // Set volume and play (SFML volume is 0-100)
        holder->sound.setVolume(volume);
        holder->sound.play();
        
        // Keep sound holder alive until it finishes playing
        g_activeSounds.push_back(std::move(holder));
        
        LOG_INFO("Playing sound: " + soundPath + " (volume: " + std::to_string(static_cast<int>(volume)) + "%)");
    } catch (const std::exception& e) {
        LOG_ERROR("Exception playing sound: " + std::string(e.what()) + " (file: " + soundPath + ")");
    } catch (...) {
        LOG_ERROR("Unknown exception playing sound: " + soundPath);
    }
}
#endif

int main() {
    // Set up crash handler FIRST, before anything else
    #ifdef _WIN32
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
    #endif
    
    // Get AppData folder for logs and stats
    std::string exeDirPath = ""; // Will store executable directory if available (for catpacks, etc.)
    std::string appDataDir = GetAppDataFolder();
    std::string logsDirPath = appDataDir;
    std::string statsDirPath = appDataDir;
    
    #ifdef _WIN32
    // Try to get executable directory for catpacks and other resources
    char exePath[MAX_PATH];
    DWORD result = GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    if (result != 0 && result < MAX_PATH) {
        try {
            std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
            exeDirPath = exeDir.string();
        } catch (...) {
            exeDirPath = "";
        }
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
    
    // Initialize SFML Audio system early to ensure it's ready
    // This helps prevent sounds not playing on first run
    try {
        // Test audio system by creating a dummy sound buffer
        // This initializes the audio subsystem
        sf::SoundBuffer testBuffer;
        LOG_INFO("SFML Audio system initialized");
    } catch (const std::exception& e) {
        LOG_WARNING("SFML Audio initialization warning: " + std::string(e.what()));
    } catch (...) {
        LOG_WARNING("SFML Audio initialization warning: unknown error");
    }
    
    // Create a small, always-on-top window (increased height for UI elements)
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(200, 260)), "Bongo Cat", 
                           sf::Style::None);
    window.setFramerateLimit(60); // Optimize performance
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
    std::string catPackConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.catpack").string();
    
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
    std::string hatConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.hat").string();
    
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
    
    // Scan for available bonk packs
    std::vector<BonkPackConfig> availableBonkPacks = BonkPackManager::scanForBonkPacks();
    if (availableBonkPacks.empty()) {
        // Add default "None" option
        availableBonkPacks.push_back(BonkPackManager::getDefaultBonkPack());
    }
    
    // Load selected bonk pack (default to "None", or load from file)
    std::string selectedBonkPackName = "None";
    std::string bonkPackConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.bonkpack").string();
    
    // Load saved bonk pack selection
    std::ifstream bonkPackFile(bonkPackConfigPath);
    if (bonkPackFile.is_open()) {
        std::getline(bonkPackFile, selectedBonkPackName);
        bonkPackFile.close();
    }
    
    // Find the selected bonk pack
    // Handle "No SFX" option specially (it's a UI-only option, not in scanned packs)
    BonkPackConfig currentBonkPack;
    if (selectedBonkPackName == "No SFX") {
        currentBonkPack = BonkPackManager::getDefaultBonkPack();
        currentBonkPack.name = "No SFX";
        LOG_INFO("Loaded bonk pack: No SFX (disabled)");
    } else {
        currentBonkPack = BonkPackManager::findBonkPackByName(availableBonkPacks, selectedBonkPackName);
        if (currentBonkPack.name != selectedBonkPackName) {
            // Selected pack not found, use "None"
            currentBonkPack = BonkPackManager::getDefaultBonkPack();
            selectedBonkPackName = currentBonkPack.name;
        } else {
            // Log the loaded bonk pack info for debugging
            LOG_INFO("Loaded bonk pack: " + currentBonkPack.name + ", bonkSound: " + currentBonkPack.bonkSound + ", folderPath: " + currentBonkPack.folderPath);
        }
    }
    
    // Create Bongo Cat with selected pack configuration
    // Use configurable cat size (default 100.0f)
    float catSize = 100.0f;
    std::string catSizeConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.catsize").string();
    
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
    
    // Function to save cat size to file
    auto saveCatSize = [&catSizeConfigPath](float size) {
        std::ofstream file(catSizeConfigPath);
        if (file.is_open()) {
            file << size;
            file.close();
        }
    };
    
    
    // Define the visual "floor" within the window (e.g., 200px down)
    // The window will be positioned so this floor aligns with the taskbar top.
    // The cat sits on this floor. The UI hangs below it.
    // Window Height is 260. Anchor is 200. UI is ~30-40px high below anchor.
    const float ANCHOR_Y = 200.0f; 
    const float WINDOW_HEIGHT = 260.0f;

    // Position cat: Center horizontally, Bottom aligned with Anchor Line
    float catX = (200.0f - catSize) / 2.0f;
    
    // Initial creation
    BongoCat bongoCat(0.0f, 0.0f, catSize, currentCatPack);
    bongoCat.setWindowHeight(WINDOW_HEIGHT); 
    bongoCat.setHat(currentHat); 
    
    // Position cat so its bottom touches the anchor line
    // This allows it to grow upwards
    float catY = ANCHOR_Y - bongoCat.getBodyDisplayHeight();
    bongoCat.setPosition(catX, catY);


    
    // Load saved UI offset or use default
    float uiOffset = 0.0f;
    std::string uiOffsetConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.uiyoffset").string();
    
    // Load saved UI offset
    std::ifstream uiOffsetFile(uiOffsetConfigPath);
    if (uiOffsetFile.is_open()) {
        std::string offsetStr;
        std::getline(uiOffsetFile, offsetStr);
        if (!offsetStr.empty()) {
            try {
                uiOffset = std::stof(offsetStr);
                if (uiOffset < -50.0f) uiOffset = -50.0f;
                if (uiOffset > 50.0f) uiOffset = 50.0f;
            } catch (...) {
                uiOffset = 0.0f;
            }
        }
        uiOffsetFile.close();
    }
    
    // Load saved UI horizontal offset or use default
    float uiHorizontalOffset = 0.0f;
    std::string uiHorizontalOffsetConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.uixoffset").string();
    
    // Load saved UI horizontal offset
    std::ifstream uiHorizontalOffsetFile(uiHorizontalOffsetConfigPath);
    if (uiHorizontalOffsetFile.is_open()) {
        std::string offsetStr;
        std::getline(uiHorizontalOffsetFile, offsetStr);
        if (!offsetStr.empty()) {
            try {
                uiHorizontalOffset = std::stof(offsetStr);
                if (uiHorizontalOffset < -50.0f) uiHorizontalOffset = -50.0f;
                if (uiHorizontalOffset > 50.0f) uiHorizontalOffset = 50.0f;
            } catch (...) {
                uiHorizontalOffset = 0.0f;
            }
        }
        uiHorizontalOffsetFile.close();
    }
    
    // Load saved SFX volume or use default
    float sfxVolume = 100.0f;
    std::string sfxVolumeConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.sfxvolume").string();
    
    // Load saved SFX volume
    std::ifstream sfxVolumeFile(sfxVolumeConfigPath);
    if (sfxVolumeFile.is_open()) {
        std::string volumeStr;
        std::getline(sfxVolumeFile, volumeStr);
        if (!volumeStr.empty()) {
            try {
                sfxVolume = std::stof(volumeStr);
                if (sfxVolume < 0.0f) sfxVolume = 0.0f;
                if (sfxVolume > 100.0f) sfxVolume = 100.0f;
            } catch (...) {
                sfxVolume = 100.0f;
            }
        }
        sfxVolumeFile.close();
    }
    
    // Load saved cat flip setting or use default
    bool catFlipped = false;
    std::string catFlipConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.catflip").string();
    
    // Load saved cat flip setting
    std::ifstream catFlipFile(catFlipConfigPath);
    if (catFlipFile.is_open()) {
        std::string flipStr;
        std::getline(catFlipFile, flipStr);
        if (!flipStr.empty()) {
            catFlipped = (flipStr == "1" || flipStr == "true");
        }
        catFlipFile.close();
    }
    
    // Apply initial cat flip
    bongoCat.setFlip(catFlipped);
    
    // Load saved arm offsets and animation vertical offset
    // These are slider values (0 = base offset for size, user can adjust from there)
    float leftArmOffset = 0.0f;
    float rightArmOffset = 0.0f;
    float animationVerticalOffset = 0.0f;
    
    // Load left arm offset
    std::string leftArmOffsetConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.leftarmoffset").string();
    std::ifstream leftArmOffsetFile(leftArmOffsetConfigPath);
    if (leftArmOffsetFile.is_open()) {
        std::string offsetStr;
        std::getline(leftArmOffsetFile, offsetStr);
        if (!offsetStr.empty()) {
            try {
                leftArmOffset = std::stof(offsetStr);
                if (leftArmOffset < -50.0f) leftArmOffset = -50.0f;
                if (leftArmOffset > 50.0f) leftArmOffset = 50.0f;
            } catch (...) {
                leftArmOffset = 0.0f;
            }
        }
        leftArmOffsetFile.close();
    }
    
    // Load right arm offset (check size-specific first)
    std::string rightArmOffsetConfigPath = (std::filesystem::path(appDataDir) / ("OpenBongo.rightarmoffset_size_" + std::to_string(static_cast<int>(catSize)))).string();
    std::ifstream rightArmOffsetFile(rightArmOffsetConfigPath);
    if (rightArmOffsetFile.is_open()) {
        std::string offsetStr;
        std::getline(rightArmOffsetFile, offsetStr);
        if (!offsetStr.empty()) {
            try {
                rightArmOffset = std::stof(offsetStr);
                if (rightArmOffset < -50.0f) rightArmOffset = -50.0f;
                if (rightArmOffset > 50.0f) rightArmOffset = 50.0f;
            } catch (...) {
                rightArmOffset = 0.0f;
            }
        }
        rightArmOffsetFile.close();
    } else {
        // Fallback to general preference
        rightArmOffsetConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.rightarmoffset").string();
        std::ifstream rightArmOffsetFileGeneral(rightArmOffsetConfigPath);
        if (rightArmOffsetFileGeneral.is_open()) {
            std::string offsetStr;
            std::getline(rightArmOffsetFileGeneral, offsetStr);
            if (!offsetStr.empty()) {
                try {
                    rightArmOffset = std::stof(offsetStr);
                    if (rightArmOffset < -50.0f) rightArmOffset = -50.0f;
                    if (rightArmOffset > 50.0f) rightArmOffset = 50.0f;
                } catch (...) {
                    rightArmOffset = 0.0f;
                }
            }
            rightArmOffsetFileGeneral.close();
        }
        // If no saved preference, use 0 (which will apply base offset for size)
    }
    
    // Load animation vertical offset (check size-specific first)
    std::string animationVerticalOffsetConfigPath = (std::filesystem::path(appDataDir) / ("OpenBongo.animationverticaloffset_size_" + std::to_string(static_cast<int>(catSize)))).string();
    std::ifstream animationVerticalOffsetFile(animationVerticalOffsetConfigPath);
    if (animationVerticalOffsetFile.is_open()) {
        std::string offsetStr;
        std::getline(animationVerticalOffsetFile, offsetStr);
        if (!offsetStr.empty()) {
            try {
                animationVerticalOffset = std::stof(offsetStr);
                if (animationVerticalOffset < -100.0f) animationVerticalOffset = -100.0f;
                if (animationVerticalOffset > 100.0f) animationVerticalOffset = 100.0f;
            } catch (...) {
                animationVerticalOffset = 0.0f;
            }
        }
        animationVerticalOffsetFile.close();
    } else {
        // Fallback to general preference
        animationVerticalOffsetConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.animationverticaloffset").string();
        std::ifstream animationVerticalOffsetFileGeneral(animationVerticalOffsetConfigPath);
        if (animationVerticalOffsetFileGeneral.is_open()) {
            std::string offsetStr;
            std::getline(animationVerticalOffsetFileGeneral, offsetStr);
            if (!offsetStr.empty()) {
                try {
                    animationVerticalOffset = std::stof(offsetStr);
                    if (animationVerticalOffset < -100.0f) animationVerticalOffset = -100.0f;
                    if (animationVerticalOffset > 100.0f) animationVerticalOffset = 100.0f;
                } catch (...) {
                    animationVerticalOffset = 0.0f;
                }
            }
            animationVerticalOffsetFileGeneral.close();
        }
        // If no saved preference, use 0 (which will apply base offset for size)
    }
    
    // Apply loaded offsets to cat (0 means base offset for size, user adjustments are relative)
    bongoCat.setLeftArmOffset(leftArmOffset);
    bongoCat.setRightArmOffset(rightArmOffset);
    bongoCat.setAnimationVerticalOffset(animationVerticalOffset);
    
    // Position window above taskbar on boot
    #ifdef _WIN32
    // Position window above taskbar on boot (using work area)

    // Get work area of primary monitor (default for new windows)
    // Get work area of primary monitor (default for new windows)
    RECT workArea = GetMonitorWorkArea(nullptr, nullptr);
    
    sf::Vector2u screenSize = sf::VideoMode::getDesktopMode().size;
    int initialX = workArea.right - 220; // 220px from right edge of work area
    
    // Position so the ANCHOR line (Y=200) aligns with the bottom of the work area (top of taskbar)
    // Formula: WindowTop = WorkAreaBottom - AnchorY
    // This places the "floor" on the taskbar.
    int initialY = workArea.bottom - static_cast<int>(ANCHOR_Y);
    
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
    bool keyboardHookInitialized = keyboardHook.initialize([&bongoCat, &totalCount, &keyStates, &currentBonkPack, &sfxVolume](unsigned int keyCode, bool isPressed) {
        try {
            if (isPressed) {
                // Only trigger if key wasn't already pressed (prevent repeat on hold)
                if (!keyStates[keyCode]) {
                    keyStates[keyCode] = true;
                    totalCount++;
                    BongoStats::getInstance().recordKeyPress(keyCode);
                    bongoCat.punch();
                    
                    // Play bonk effect SFX if not "None" or "No SFX"
                    if (currentBonkPack.name != "None" && currentBonkPack.name != "No SFX") {
                        if (currentBonkPack.bonkSound.empty()) {
                            LOG_WARNING("Key pressed but bonkSound is empty for pack: " + currentBonkPack.name);
                        } else if (currentBonkPack.folderPath.empty()) {
                            LOG_WARNING("Key pressed but folderPath is empty for pack: " + currentBonkPack.name);
                        } else {
                            std::string bonkSoundPath = currentBonkPack.getSoundPath(currentBonkPack.bonkSound);
                            if (bonkSoundPath.empty()) {
                                LOG_WARNING("Key pressed but sound path is empty (pack: " + currentBonkPack.name + ", sound: " + currentBonkPack.bonkSound + ", folder: " + currentBonkPack.folderPath + ")");
                            } else {
                                // Verify file exists before attempting to play
                                if (std::filesystem::exists(bonkSoundPath)) {
                                    LOG_INFO("Key pressed - Playing bonk sound: " + bonkSoundPath);
                                    PlaySoundFile(bonkSoundPath, sfxVolume);
                                } else {
                                    LOG_WARNING("Key pressed but bonk sound file not found: " + bonkSoundPath + " (pack: " + currentBonkPack.name + ", folder: " + currentBonkPack.folderPath + ")");
                                }
                            }
                        }
                    }
                    // Don't log for None/No SFX to avoid spam
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
                    // Check if cursor is on the taskbar (outside work area but inside monitor bounds)
                    HMONITOR hMonitor = MonitorFromPoint(cursorPos, MONITOR_DEFAULTTONEAREST);
                    MONITORINFO mi = { sizeof(MONITORINFO) };
                    
                    if (GetMonitorInfo(hMonitor, &mi)) {
                        // Check if cursor is outside work area (which implies it's on a docked bar like taskbar)
                        // but still inside the monitor rect
                        bool insideMonitor = PtInRect(&mi.rcMonitor, cursorPos);
                        bool insideWorkArea = PtInRect(&mi.rcWork, cursorPos);
                        
                        // If on monitor but not in work area, assume taskbar/dock interaction
                        // Just checking bottom edge for now as that's where most errors occur
                        if (insideMonitor && !insideWorkArea) {
                             taskbarWasClicked = true;
                        } 
                        else if (taskbarWasClicked) {
                            // Was clicking taskbar/outside, now clicked inside work area - snap!
                            // Snap so the ANCHOR line aligns with work area bottom
                            // This effectively "lands" the cat on the taskbar
                            int newX = window.getPosition().x;
                            // Re-use logic: WindowTop = WorkAreaBottom - 200
                            int newY = mi.rcWork.bottom - 200;
                            
                            window.setPosition(sf::Vector2i(newX, newY));
                            taskbarWasClicked = false;
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
    // uiY will be recalculated in the main loop to account for offset
    float uiY = bongoCat.getBodyBottomY() + 10.0f - uiOffset; // Position below cat with offset (inverted)
    float counterBoxWidth = 84.0f; // 120.0f * 0.7
    float counterBoxHeight = 21.0f; // 30.0f * 0.7
    float menuButtonSize = 21.0f; // 30.0f * 0.7
    float spacing = 10.0f; // Spacing between counter and menu button
    float totalUIWidth = counterBoxWidth + spacing + menuButtonSize;
    float windowWidth = 200.0f;
    // Center the UI elements relative to the window (which centers them relative to the cat)
    // Apply horizontal offset
    float counterBoxX = (windowWidth - totalUIWidth) / 2.0f + uiHorizontalOffset;
    float menuButtonX = counterBoxX + counterBoxWidth + spacing;
    
    // Hamburger menu line dimensions (needed in main loop)
    float lineWidth = 12.6f; // 18.0f * 0.7
    float lineHeight = 1.4f; // 2.0f * 0.7
    float lineSpacing = 2.8f; // 4.0f * 0.7
    
    // Counter box (light grey background)
    counterBoxPtr = std::make_unique<sf::RectangleShape>(sf::Vector2f(counterBoxWidth, counterBoxHeight));
    counterBoxPtr->setPosition(sf::Vector2f(counterBoxX, uiY));
    counterBoxPtr->setFillColor(sf::Color(220, 220, 220)); // Light grey
    
    // Menu button (darker grey square)
    menuButtonPtr = std::make_unique<sf::RectangleShape>(sf::Vector2f(menuButtonSize, menuButtonSize));
    menuButtonPtr->setPosition(sf::Vector2f(menuButtonX, uiY));
    menuButtonPtr->setFillColor(sf::Color(180, 180, 180)); // Darker grey
    
    // Hamburger menu lines (three horizontal lines)
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
        try {
            // Check if webview is valid before sending
            if (!webView.isWindowValid()) {
                LOG_WARNING("WebView window is invalid, cannot send message: " + type);
                return;
            }
            // Create properly formatted JSON message
            std::string message = "{\"type\":\"" + type + "\",\"data\":" + jsonData + "}";
            webView.postMessage(message);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in sendJSONToWebView: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in sendJSONToWebView");
        }
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
            
            // Find catpacks, hats, bonk-packs, or entity-sfx in the path (case-insensitive search)
            std::string pathLower = pathStr;
            std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), ::tolower);
            
            size_t catpacksPos = pathLower.find("catpacks/");
            size_t hatsPos = pathLower.find("hats/");
            size_t bonkPacksPos = pathLower.find("bonk-packs/");
            
            std::string relativePath;
            if (catpacksPos != std::string::npos) {
                // Extract from "catpacks/" onwards
                relativePath = pathStr.substr(catpacksPos);
            } else if (hatsPos != std::string::npos) {
                // Extract from "hats/" onwards
                relativePath = pathStr.substr(hatsPos);
            } else if (bonkPacksPos != std::string::npos) {
                // Extract from "bonk-packs/" onwards
                relativePath = pathStr.substr(bonkPacksPos);
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
    
    // Helper function to send bonk pack list
    auto sendBonkPackList = [&sendJSONToWebView, &escapeJSON, &pathToUrl](WebViewWindow& webView, const std::vector<BonkPackConfig>& packs) {
        try {
            std::stringstream ss;
            ss << "[";
            for (size_t i = 0; i < packs.size(); i++) {
                if (i > 0) ss << ",";
                try {
                    std::string iconPath = packs[i].getImagePath(packs[i].iconImage);
                    std::string iconUrl = iconPath.empty() ? "" : pathToUrl(iconPath, 18080);
                    ss << "{\"name\":\"" << escapeJSON(packs[i].name) << "\",\"iconPath\":\"" << escapeJSON(iconUrl) << "\"}";
                } catch (const std::exception& e) {
                    LOG_ERROR("Error processing bonk pack at index " + std::to_string(i) + ": " + e.what());
                    // Skip this pack and continue
                    continue;
                } catch (...) {
                    LOG_ERROR("Unknown error processing bonk pack at index " + std::to_string(i));
                    continue;
                }
            }
            ss << "]";
            std::string jsonStr = ss.str();
            LOG_INFO("Sending bonk pack list: " + jsonStr);
            sendJSONToWebView(webView, "bonkPackList", jsonStr);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in sendBonkPackList: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("Unknown exception in sendBonkPackList");
        }
    };
    
    auto sendSelectedBonkPack = [&sendJSONToWebView, &escapeJSON](WebViewWindow& webView, const BonkPackConfig& pack) {
        std::string json = "{\"name\":\"" + escapeJSON(pack.name) + "\"}";
        LOG_INFO("Sending selected bonk pack: " + json);
        sendJSONToWebView(webView, "selectedBonkPack", json);
    };
    
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
    // Flag to track if window needs to be recreated (for size changes)
    bool shouldRecreateWindow = false;

    int loopIteration = 0;
    
    while (window.isOpen() || shouldRecreateWindow) {
        // Handle window recreation if requested
        if (shouldRecreateWindow) {
            // Get current position to restore it
            sf::Vector2i currentPos = window.getPosition();
            
            // Recreate window
            window.create(sf::VideoMode(sf::Vector2u(200, 260)), "Bongo Cat", sf::Style::None);
            window.setFramerateLimit(60);
            
            // Restore position
            window.setPosition(currentPos);
            
            // Re-apply window styles
            #ifdef _WIN32
            hwnd = window.getNativeHandle();
            if (hwnd) {
                // Set extended style to include topmost and layered window
                LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                exStyle |= WS_EX_LAYERED | WS_EX_TOPMOST;
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
                
                // Set window to be always on top
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                
                // Set transprency color key
                SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
                
                // Add system menu
                LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
                style |= WS_SYSMENU;
                SetWindowLongPtr(hwnd, GWL_STYLE, style);
            }
            #endif
            
            shouldRecreateWindow = false;
            LOG_INFO("Window recreated successfully");
        }
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
                                        // Check if webview is still valid before processing
                                        if (!settingsWebView || !settingsWebView->isWindowValid()) {
                                            LOG_WARNING("WebView is invalid, ignoring message");
                                            return;
                                        }
                                        
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
                                        } else if (type == "getBonkPacks") {
                                            LOG_INFO("Sending bonk packs list");
                                            sendBonkPackList(*settingsWebView, availableBonkPacks);
                                        } else if (type == "getSelectedCatPack") {
                                            LOG_INFO("Sending selected cat pack");
                                            sendSelectedCatPack(*settingsWebView, currentCatPack);
                                        } else if (type == "getSelectedHat") {
                                            LOG_INFO("Sending selected hat");
                                            sendSelectedHat(*settingsWebView, currentHat);
                                        } else if (type == "getSelectedBonkPack") {
                                            LOG_INFO("Sending selected bonk pack");
                                            sendSelectedBonkPack(*settingsWebView, currentBonkPack);
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
                                        } else if (type == "selectBonkPack") {
                                            std::string packName = parsed["name"];
                                            // Handle "No SFX" option
                                            if (packName == "No SFX") {
                                                selectedBonkPackName = "No SFX";
                                                currentBonkPack = BonkPackManager::getDefaultBonkPack();
                                                currentBonkPack.name = "No SFX";
                                                
                                                LOG_INFO("Bonk pack selected: No SFX (disabled)");
                                                
                                                // Save selection to AppData
                                                std::ofstream bonkPackOutFile(bonkPackConfigPath);
                                                if (bonkPackOutFile.is_open()) {
                                                    bonkPackOutFile << selectedBonkPackName;
                                                    bonkPackOutFile.close();
                                                    LOG_INFO("Bonk pack selection saved to: " + bonkPackConfigPath);
                                                } else {
                                                    LOG_ERROR("Failed to save bonk pack selection to: " + bonkPackConfigPath);
                                                }
                                                
                                                sendSelectedBonkPack(*settingsWebView, currentBonkPack);
                                            } else {
                                                BonkPackConfig newBonkPack = BonkPackManager::findBonkPackByName(availableBonkPacks, packName);
                                                if (newBonkPack.name == packName) {
                                                    selectedBonkPackName = packName;
                                                    currentBonkPack = newBonkPack;
                                                    
                                                    LOG_INFO("Bonk pack selected: " + packName + ", bonkSound: " + currentBonkPack.bonkSound + ", folderPath: " + currentBonkPack.folderPath);
                                                    
                                                    // Verify the sound file exists
                                                    if (currentBonkPack.bonkSound.empty()) {
                                                        LOG_WARNING("Bonk sound filename is empty for pack: " + packName);
                                                    } else if (currentBonkPack.folderPath.empty()) {
                                                        LOG_WARNING("Folder path is empty for pack: " + packName);
                                                    } else {
                                                        std::string testSoundPath = currentBonkPack.getSoundPath(currentBonkPack.bonkSound);
                                                        LOG_INFO("Constructed sound path: " + testSoundPath);
                                                        if (std::filesystem::exists(testSoundPath)) {
                                                            LOG_INFO("Bonk sound file verified and exists: " + testSoundPath);
                                                        } else {
                                                            LOG_WARNING("Bonk sound file NOT FOUND: " + testSoundPath);
                                                            // Try to list directory contents for debugging
                                                            try {
                                                                if (std::filesystem::exists(currentBonkPack.folderPath)) {
                                                                    LOG_INFO("Folder exists, listing contents:");
                                                                    for (const auto& entry : std::filesystem::directory_iterator(currentBonkPack.folderPath)) {
                                                                        LOG_INFO("  - " + entry.path().filename().string());
                                                                    }
                                                                } else {
                                                                    LOG_WARNING("Folder does not exist: " + currentBonkPack.folderPath);
                                                                }
                                                            } catch (const std::exception& e) {
                                                                LOG_ERROR("Error listing folder contents: " + std::string(e.what()));
                                                            }
                                                        }
                                                    }
                                                    
                                                    // Save selection to AppData
                                                    std::ofstream bonkPackOutFile(bonkPackConfigPath);
                                                    if (bonkPackOutFile.is_open()) {
                                                        bonkPackOutFile << selectedBonkPackName;
                                                        bonkPackOutFile.close();
                                                        LOG_INFO("Bonk pack selection saved to: " + bonkPackConfigPath);
                                                    } else {
                                                        LOG_ERROR("Failed to save bonk pack selection to: " + bonkPackConfigPath);
                                                    }
                                                    
                                                    sendSelectedBonkPack(*settingsWebView, currentBonkPack);
                                                } else {
                                                    LOG_WARNING("Bonk pack not found: " + packName);
                                                }
                                            }

                                            // Extract color from message
                                            std::regex colorRegex("\"color\"\\s*:\\s*\"([^\"]+)\"");
                                            std::smatch colorMatch;
                                            if (std::regex_search(message, colorMatch, colorRegex)) {
                                                std::string color = colorMatch[1].str();
                                                LOG_INFO("Accent color changed to: " + color);
                                                
                                                // Save accent color to file
                                                std::string accentColorConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.accentcolor").string();
                                                
                                                std::ofstream accentColorOutFile(accentColorConfigPath);
                                                if (accentColorOutFile.is_open()) {
                                                    accentColorOutFile << color;
                                                    accentColorOutFile.close();
                                                    LOG_INFO("Accent color saved: " + color);
                                                }
                                            }
                                        } else if (type == "setCatSize") {
                                            // Extract size from message
                                            std::regex sizeRegex("\"size\"\\s*:\\s*(\\d+)");
                                            std::smatch sizeMatch;
                                            if (std::regex_search(message, sizeMatch, sizeRegex)) {
                                                try {
                                                    int newSizeInt = std::stoi(sizeMatch[1].str());
                                                    float newSize = static_cast<float>(newSizeInt);
                                                    
                                                    if (newSize >= 50.0f && newSize <= 200.0f) {
                                                        // Save the new size
                                                        catSize = newSize;
                                                        saveCatSize(catSize);
                                                        LOG_INFO("Saved cat size to: " + std::to_string(catSize));
                                                        
                                                        // Send restart message to UI
                                                        if (settingsWebView && settingsWebView->isWindowValid()) {
                                                            std::string restartJson = "{\"message\":\"Please restart OpenBongo for this change to take effect.\"}";
                                                            sendJSONToWebView(*settingsWebView, "restartRequired", restartJson);
                                                        }
                                                    }
                                                } catch (...) {
                                                    LOG_ERROR("Failed to parse cat size");
                                                }
                                            }
                                        } else if (type == "setUIOffset") {
                                            // Extract offset from message
                                            std::regex offsetRegex("\"offset\"\\s*:\\s*(-?\\d+)");
                                            std::smatch offsetMatch;
                                            if (std::regex_search(message, offsetMatch, offsetRegex)) {
                                                try {
                                                    float newOffset = std::stof(offsetMatch[1].str());
                                                    if (newOffset >= -50.0f && newOffset <= 50.0f) {
                                                        uiOffset = newOffset;
                                                        
                                                        // Save UI offset to file
                                                        std::ofstream uiOffsetOutFile(uiOffsetConfigPath);
                                                        if (uiOffsetOutFile.is_open()) {
                                                            uiOffsetOutFile << uiOffset;
                                                            uiOffsetOutFile.close();
                                                            LOG_INFO("UI offset saved: " + std::to_string(uiOffset));
                                                        }
                                                    }
                                                } catch (...) {
                                                    LOG_ERROR("Failed to parse UI offset");
                                                }
                                            }
                                        } else if (type == "setUIHorizontalOffset") {
                                            // Extract horizontal offset from message
                                            std::regex offsetRegex("\"offset\"\\s*:\\s*(-?\\d+)");
                                            std::smatch offsetMatch;
                                            if (std::regex_search(message, offsetMatch, offsetRegex)) {
                                                try {
                                                    float newOffset = std::stof(offsetMatch[1].str());
                                                    if (newOffset >= -50.0f && newOffset <= 50.0f) {
                                                        uiHorizontalOffset = newOffset;
                                                        
                                                        // Save UI horizontal offset to file
                                                        std::ofstream uiHorizontalOffsetOutFile(uiHorizontalOffsetConfigPath);
                                                        if (uiHorizontalOffsetOutFile.is_open()) {
                                                            uiHorizontalOffsetOutFile << uiHorizontalOffset;
                                                            uiHorizontalOffsetOutFile.close();
                                                            LOG_INFO("UI horizontal offset saved: " + std::to_string(uiHorizontalOffset));
                                                        }
                                                    }
                                                } catch (...) {
                                                    LOG_ERROR("Failed to parse UI horizontal offset");
                                                }
                                            }
                                        } else if (type == "setSFXVolume") {
                                            // Extract volume from message
                                            std::regex volumeRegex("\"volume\"\\s*:\\s*(\\d+)");
                                            std::smatch volumeMatch;
                                            if (std::regex_search(message, volumeMatch, volumeRegex)) {
                                                try {
                                                    float newVolume = std::stof(volumeMatch[1].str());
                                                    if (newVolume >= 0.0f && newVolume <= 100.0f) {
                                                        sfxVolume = newVolume;
                                                        
                                                        // Save SFX volume to file
                                                        std::ofstream sfxVolumeOutFile(sfxVolumeConfigPath);
                                                        if (sfxVolumeOutFile.is_open()) {
                                                            sfxVolumeOutFile << sfxVolume;
                                                            sfxVolumeOutFile.close();
                                                            LOG_INFO("SFX volume saved: " + std::to_string(sfxVolume));
                                                        }
                                                    }
                                                } catch (...) {
                                                    LOG_ERROR("Failed to parse SFX volume");
                                                }
                                            }
                                        } else if (type == "setCatFlip") {
                                            // Extract flipped state from message
                                            std::regex flipRegex("\"flipped\"\\s*:\\s*(true|false)");
                                            std::smatch flipMatch;
                                            if (std::regex_search(message, flipMatch, flipRegex)) {
                                                bool newFlipped = (flipMatch[1].str() == "true");
                                                catFlipped = newFlipped;
                                                bongoCat.setFlip(catFlipped);
                                                LOG_INFO("Cat flip changed to: " + std::string(catFlipped ? "true" : "false"));
                                                
                                                // Save cat flip to file
                                                std::ofstream catFlipOutFile(catFlipConfigPath);
                                                if (catFlipOutFile.is_open()) {
                                                    catFlipOutFile << (catFlipped ? "1" : "0");
                                                    catFlipOutFile.close();
                                                    LOG_INFO("Cat flip saved: " + std::string(catFlipped ? "true" : "false"));
                                                }
                                            }
                                        } else if (type == "setLeftArmOffset") {
                                            // Extract left arm offset from message
                                            std::regex offsetRegex("\"offset\"\\s*:\\s*([+-]?\\d+(?:\\.\\d+)?)");
                                            std::smatch offsetMatch;
                                            if (std::regex_search(message, offsetMatch, offsetRegex)) {
                                                try {
                                                    float offset = std::stof(offsetMatch[1].str());
                                                    if (offset >= -50.0f && offset <= 50.0f) {
                                                        bongoCat.setLeftArmOffset(offset);
                                                        LOG_INFO("Left arm offset set to: " + std::to_string(offset));
                                                        
                                                        // Save left arm offset to file
                                                        std::string leftArmOffsetConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.leftarmoffset").string();
                                                        std::ofstream leftArmOffsetOutFile(leftArmOffsetConfigPath);
                                                        if (leftArmOffsetOutFile.is_open()) {
                                                            leftArmOffsetOutFile << offset;
                                                            leftArmOffsetOutFile.close();
                                                            LOG_INFO("Left arm offset saved: " + std::to_string(offset));
                                                        }
                                                    }
                                                } catch (...) {
                                                    LOG_ERROR("Failed to parse left arm offset");
                                                }
                                            }
                                        } else if (type == "setRightArmOffset") {
                                            // Extract right arm offset from message
                                            std::regex offsetRegex("\"offset\"\\s*:\\s*([+-]?\\d+(?:\\.\\d+)?)");
                                            std::smatch offsetMatch;
                                            if (std::regex_search(message, offsetMatch, offsetRegex)) {
                                                try {
                                                    float offset = std::stof(offsetMatch[1].str());
                                                    if (offset >= -50.0f && offset <= 50.0f) {
                                                        bongoCat.setRightArmOffset(offset);
                                                        LOG_INFO("Right arm offset set to: " + std::to_string(offset));
                                                        
                                                        // Save right arm offset to file (size-specific)
                                                        std::string rightArmOffsetConfigPath = (std::filesystem::path(appDataDir) / ("OpenBongo.rightarmoffset_size_" + std::to_string(static_cast<int>(catSize)))).string();
                                                        std::ofstream rightArmOffsetOutFile(rightArmOffsetConfigPath);
                                                        if (rightArmOffsetOutFile.is_open()) {
                                                            rightArmOffsetOutFile << offset;
                                                            rightArmOffsetOutFile.close();
                                                            LOG_INFO("Right arm offset saved for size " + std::to_string(static_cast<int>(catSize)) + ": " + std::to_string(offset));
                                                        }
                                                    }
                                                } catch (...) {
                                                    LOG_ERROR("Failed to parse right arm offset");
                                                }
                                            }
                                        } else if (type == "setAnimationVerticalOffset") {
                                            // Extract animation vertical offset from message
                                            std::regex offsetRegex("\"offset\"\\s*:\\s*([+-]?\\d+(?:\\.\\d+)?)");
                                            std::smatch offsetMatch;
                                            if (std::regex_search(message, offsetMatch, offsetRegex)) {
                                                try {
                                                    float offset = std::stof(offsetMatch[1].str());
                                                    if (offset >= -100.0f && offset <= 100.0f) {
                                                        bongoCat.setAnimationVerticalOffset(offset);
                                                        LOG_INFO("Animation vertical offset set to: " + std::to_string(offset));
                                                        
                                                        // Save animation vertical offset to file (size-specific)
                                                        std::string animationVerticalOffsetConfigPath = (std::filesystem::path(appDataDir) / ("OpenBongo.animationverticaloffset_size_" + std::to_string(static_cast<int>(catSize)))).string();
                                                        std::ofstream animationVerticalOffsetOutFile(animationVerticalOffsetConfigPath);
                                                        if (animationVerticalOffsetOutFile.is_open()) {
                                                            animationVerticalOffsetOutFile << offset;
                                                            animationVerticalOffsetOutFile.close();
                                                            LOG_INFO("Animation vertical offset saved for size " + std::to_string(static_cast<int>(catSize)) + ": " + std::to_string(offset));
                                                        }
                                                    }
                                                } catch (...) {
                                                    LOG_ERROR("Failed to parse animation vertical offset");
                                                }
                                            }
                                        } else if (type == "shutdown") {
                                            // Shutdown the entire program
                                            LOG_INFO("Shutdown requested from UI");
                                            window.close();
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
                                    sendBonkPackList(*settingsWebView, availableBonkPacks);
                                    sendSelectedCatPack(*settingsWebView, currentCatPack);
                                    sendSelectedHat(*settingsWebView, currentHat);
                                    sendSelectedBonkPack(*settingsWebView, currentBonkPack);
                                    
                                    // Also send bonk packs again after a short delay to ensure they're received
                                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                    sendBonkPackList(*settingsWebView, availableBonkPacks);
                                    
                                    // Send initial cat size
                                    std::string sizeJson = "{\"size\":" + std::to_string(static_cast<int>(catSize)) + "}";
                                    sendJSONToWebView(*settingsWebView, "catSize", sizeJson);
                                    

                                    
                                    // Send initial UI offset
                                    std::string uiOffsetJson = "{\"offset\":" + std::to_string(static_cast<int>(uiOffset)) + "}";
                                    sendJSONToWebView(*settingsWebView, "uiOffset", uiOffsetJson);
                                    
                                    // Send initial UI horizontal offset
                                    std::string uiHorizontalOffsetJson = "{\"offset\":" + std::to_string(static_cast<int>(uiHorizontalOffset)) + "}";
                                    sendJSONToWebView(*settingsWebView, "uiHorizontalOffset", uiHorizontalOffsetJson);
                                    
                                    // Send initial SFX volume
                                    std::string sfxVolumeJson = "{\"volume\":" + std::to_string(static_cast<int>(sfxVolume)) + "}";
                                    sendJSONToWebView(*settingsWebView, "sfxVolume", sfxVolumeJson);
                                    
                                    // Send initial cat flip
                                    std::string catFlipJson = "{\"flipped\":" + std::string(catFlipped ? "true" : "false") + "}";
                                    sendJSONToWebView(*settingsWebView, "catFlip", catFlipJson);
                                    
                                    // Load and send initial accent color
                                    std::string accentColorConfigPath = (std::filesystem::path(appDataDir) / "OpenBongo.accentcolor").string();
                                    
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
                        
                        // Get work area for the window/monitor
                        RECT workArea = GetMonitorWorkArea(window.getNativeHandle(), nullptr);
                        
                        // Check if the ANCHOR LINE (newY + 200) effectively touches or crosses the bottom
                        // Actually, let's essentially snap the ANCHOR line to the bottom.
                        // Anchor is at Y=200 relative to window top.
                        // Screen Y of Anchor = newY + 200.
                        
                        // If (Screen Y of Anchor) >= WorkAreaBottom, then Snap.
                        if ((newY + 200) >= workArea.bottom) {
                             newY = workArea.bottom - 200;
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
        
        // Recalculate UI Y position with offset (inverted: negative moves up, positive moves down)
        float currentUIY = bongoCat.getBodyBottomY() + 10.0f - uiOffset;
        
        // Recalculate UI X positions with horizontal offset
        float currentCounterBoxX = (windowWidth - totalUIWidth) / 2.0f + uiHorizontalOffset;
        float currentMenuButtonX = currentCounterBoxX + counterBoxWidth + spacing;
        
        // Update UI element positions if they changed
        bool uiPositionChanged = false;
        if (std::abs(currentUIY - uiY) > 0.01f) {
            uiY = currentUIY;
            uiPositionChanged = true;
        }
        if (std::abs(currentCounterBoxX - counterBoxX) > 0.01f) {
            counterBoxX = currentCounterBoxX;
            menuButtonX = currentMenuButtonX;
            uiPositionChanged = true;
        }
        
        if (uiPositionChanged) {
            // Update counter box position
            if (counterBoxPtr) {
                counterBoxPtr->setPosition(sf::Vector2f(counterBoxX, uiY));
            }
            
            // Update menu button position
            if (menuButtonPtr) {
                menuButtonPtr->setPosition(sf::Vector2f(menuButtonX, uiY));
            }
            
            // Update hamburger menu lines position
            float firstLineY = uiY + (menuButtonSize - (lineHeight * 3 + lineSpacing * 2)) / 2.0f;
            if (menuButtonLine1Ptr) {
                menuButtonLine1Ptr->setPosition(sf::Vector2f(menuButtonX + (menuButtonSize - lineWidth) / 2.0f, firstLineY));
            }
            if (menuButtonLine2Ptr) {
                menuButtonLine2Ptr->setPosition(sf::Vector2f(menuButtonX + (menuButtonSize - lineWidth) / 2.0f, firstLineY + lineHeight + lineSpacing));
            }
            if (menuButtonLine3Ptr) {
                menuButtonLine3Ptr->setPosition(sf::Vector2f(menuButtonX + (menuButtonSize - lineWidth) / 2.0f, firstLineY + (lineHeight + lineSpacing) * 2));
            }
        }
        
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


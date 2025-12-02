// This is a reference implementation showing how to integrate WebView2
// The actual main.cpp needs to be updated with these changes

// Key changes needed in main.cpp:
// 1. Replace #include <SFML/Graphics.hpp> with WebView2 includes
// 2. Replace sf::RenderWindow with WebViewWindow
// 3. Replace rendering loop with message sending
// 4. Add JSON message handling

// Example WebView2 integration (to be merged into main.cpp):

#ifdef _WIN32
#include "ui/WebViewWindow.h"
#include "utils/ImageHelper.h"
#include <json/json.h> // You'll need a JSON library like jsoncpp or nlohmann/json
#endif

// In main(), replace window creation:
/*
#ifdef _WIN32
    // Create WebView2 window instead of SFML window
    WebViewWindow webViewWindow;
    std::string htmlPath = (std::filesystem::path(exeDirPath) / "ui" / "index.html").string();
    if (!std::filesystem::exists(htmlPath)) {
        htmlPath = "ui/index.html"; // Fallback
    }
    
    if (!webViewWindow.initialize(nullptr, htmlPath)) {
        LOG_ERROR("Failed to initialize WebView2 window");
        return 1;
    }
    
    HWND hwnd = webViewWindow.getHwnd();
    
    // Set up message handler
    webViewWindow.setMessageHandler([&](const std::string& message) {
        // Parse JSON message from JavaScript
        // Handle different message types
        // Update backend state
        // Send responses back
    });
    
    // Send initial data to JavaScript
    sendCounterUpdate(webViewWindow, totalCount);
    sendCatPackList(webViewWindow, availableCatPacks);
    sendHatList(webViewWindow, availableHats);
    sendSelectedCatPack(webViewWindow, currentCatPack);
    sendSelectedHat(webViewWindow, currentHat);
    
    // Send cat images
    sendCatImages(webViewWindow, bongoCat);
#endif
*/

// Replace main loop:
/*
    while (webViewWindow.isInitialized()) {
        // Update cat animation
        bongoCat.update(deltaTime);
        
        // Send updated counter
        if (counterChanged) {
            sendCounterUpdate(webViewWindow, totalCount);
        }
        
        // Send punch animation trigger
        if (punchTriggered) {
            sendPunchMessage(webViewWindow);
        }
        
        // Send cat position data
        sendCatData(webViewWindow, bongoCat);
        
        // Process Windows messages
        webViewWindow.runMessageLoop();
        
        // Small delay to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
    }
*/

// Helper functions to send messages:
/*
void sendCounterUpdate(WebViewWindow& window, int count) {
    std::string json = R"({"type":"counterUpdate","data":{"count":)" + std::to_string(count) + "}}";
    window.postMessage(json);
}

void sendPunchMessage(WebViewWindow& window) {
    std::string json = R"({"type":"punch"})";
    window.postMessage(json);
}

void sendCatData(WebViewWindow& window, BongoCat& cat) {
    // Get cat position and state
    auto pos = cat.getPosition();
    float bodyBottomY = cat.getBodyBottomY();
    
    std::string json = R"({"type":"catData","data":{)"
        + R"("bodyX":)" + std::to_string(pos.x) + ","
        + R"("bodyY":)" + std::to_string(pos.y) + ","
        + R"("bodySize":100.0})";
    window.postMessage(json);
}

void sendCatImages(WebViewWindow& window, BongoCat& cat) {
    // This requires access to BongoCat's internal textures
    // You may need to add methods to BongoCat to get image data
    // For now, load images directly from files
    CatPackConfig config = cat.getConfig(); // Need to add getConfig() method
    
    std::string bodyPath = config.getImagePath(config.bodyImage);
    std::string handUpPath = config.getImagePath(config.handUpImage);
    std::string handDownPath = config.getImagePath(config.handDownImage);
    
    std::string bodyBase64 = ImageHelper::imageFileToBase64(bodyPath);
    std::string handUpBase64 = ImageHelper::imageFileToBase64(handUpPath);
    std::string handDownBase64 = ImageHelper::imageFileToBase64(handDownPath);
    
    // Send body image
    std::string json = R"({"type":"imageData","data":{"type":"body","base64":")" + bodyBase64 + R"("}})";
    window.postMessage(json);
    
    // Send hand images
    json = R"({"type":"imageData","data":{"type":"handUp","base64":")" + handUpBase64 + R"("}})";
    window.postMessage(json);
    
    json = R"({"type":"imageData","data":{"type":"handDown","base64":")" + handDownBase64 + R"("}})";
    window.postMessage(json);
}
*/


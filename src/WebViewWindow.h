#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

// Use Crow (C++ HTTP framework) + webview for HTML UI
// Crow: https://github.com/CrowCpp/Crow (BSD license, free and open source)
// webview: https://github.com/webview/webview (MIT license, free and open source)
#if __has_include("crow.h")
#include "crow.h"
#elif __has_include("crow/crow.h")
#include "crow/crow.h"
#elif __has_include("third_party/crow/include/crow.h")
#include "third_party/crow/include/crow.h"
#else
// Forward declarations
namespace crow {
    class SimpleApp;
}
#endif

#if __has_include("webview/webview.h")
#include "webview/webview.h"
#elif __has_include("third_party/webview/core/include/webview/webview.h")
#include "third_party/webview/core/include/webview/webview.h"
#else
// Forward declarations for C API
struct webview;
typedef void* webview_t;
#endif

class WebViewWindow {
public:
    WebViewWindow();
    ~WebViewWindow();
    
    bool initialize(void* parentHwnd, const std::string& htmlPath);
    void navigateToFile(const std::string& htmlPath);
    void postMessage(const std::string& message);
    void setMessageHandler(std::function<void(const std::string&)> handler);
    void runMessageLoop();
    void shutdown();
    
    void* getHwnd() const;
    bool isInitialized() const { return m_initialized; }
    bool isWindowValid() const;
    void hideWindow();
    void showWindow();
    
private:
    webview_t m_webview;
    std::unique_ptr<crow::SimpleApp> m_app;
    std::thread m_serverThread;
    std::atomic<bool> m_serverRunning;
    std::atomic<int> m_serverPort;
    std::function<void(const std::string&)> m_messageHandler;
    bool m_initialized;
    std::atomic<bool> m_shuttingDown;
    std::string m_htmlPath;
    std::string m_uiDirectory;
    
    void startServer();
    void stopServer();
    static void messageCallback(const char* seq, const char* req, void* userData);
};

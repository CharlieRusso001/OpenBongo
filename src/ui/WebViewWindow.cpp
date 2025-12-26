#include "ui/WebViewWindow.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

// Include Crow
#if __has_include("crow.h")
#include "crow.h"
#elif __has_include("crow/crow.h")
#include "crow/crow.h"
#elif __has_include("third_party/crow/include/crow.h")
#include "third_party/crow/include/crow.h"
#else
#include "crow.h"
#endif

// Include webview
#if __has_include("webview/webview.h")
#include "webview/webview.h"
#elif __has_include("third_party/webview/core/include/webview/webview.h")
#include "third_party/webview/core/include/webview/webview.h"
#else
#include "webview/webview.h"
#endif

WebViewWindow* g_webViewInstance = nullptr;

// Helper function for webview_dispatch to evaluate JavaScript
static void evalScriptCallback(webview_t w, void* arg) {
    std::string* script = static_cast<std::string*>(arg);
    if (script && w) {
        webview_eval(w, script->c_str());
    }
    delete script; // Clean up allocated string
}

WebViewWindow::WebViewWindow() 
    : m_webview(nullptr), m_app(nullptr), m_serverRunning(false), m_serverPort(0), m_initialized(false), m_shuttingDown(false) {
    g_webViewInstance = this;
}

WebViewWindow::~WebViewWindow() {
    shutdown();
    g_webViewInstance = nullptr;
}

void WebViewWindow::messageCallback(const char* seq, const char* req, void* userData) {
    WebViewWindow* instance = static_cast<WebViewWindow*>(userData);
    if (instance && instance->m_messageHandler && req) {
        instance->m_messageHandler(std::string(req));
    }
}

bool WebViewWindow::initialize(void* parentHwnd, const std::string& htmlPath) {
    m_htmlPath = htmlPath;
    
    // Get UI directory (parent of HTML file)
    std::filesystem::path htmlFilePath(htmlPath);
    m_uiDirectory = htmlFilePath.parent_path().string();
    
    try {
        // Start Crow HTTP server in a separate thread
        startServer();
        
        // Wait for server to start (poll with timeout instead of fixed sleep)
        auto startTime = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::milliseconds(500);
        while ((std::chrono::steady_clock::now() - startTime) < timeout) {
            if (m_serverRunning && m_serverPort > 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (!m_serverRunning || m_serverPort == 0) {
            std::cerr << "Failed to start Crow server" << std::endl;
            return false;
        }
        
        // Create webview pointing to localhost
        m_webview = webview_create(0, parentHwnd);
        if (!m_webview) {
            std::cerr << "Failed to create webview" << std::endl;
            stopServer();
            return false;
        }
        
        // Set window properties
        webview_set_title(m_webview, "OpenBongo Settings");
        webview_set_size(m_webview, 650, 600, WEBVIEW_HINT_FIXED);
        
        // Bind message handler
        webview_bind(m_webview, "postMessage", messageCallback, this);
        
        // Navigate to localhost
        std::string url = "http://localhost:" + std::to_string(m_serverPort) + "/";
        webview_navigate(m_webview, url.c_str());
        
        // Inject build version info
        std::string buildDate = __DATE__;
        std::string buildTime = __TIME__;
        std::string versionScript = "setTimeout(function() { try { var el = document.getElementById('version-display'); if(el) el.innerText = 'Build: " + buildDate + " " + buildTime + "'; } catch(e) {} }, 1000);"; // Delay slightly to ensure DOM is ready
        webview_dispatch(m_webview, evalScriptCallback, new std::string(versionScript));
        
        m_initialized = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize WebViewWindow: " << e.what() << std::endl;
        return false;
    }
}

void WebViewWindow::startServer() {
    // Ensure any existing server is fully stopped before starting a new one
    if (m_serverThread.joinable() || m_app) {
        stopServer();
        // Give a small delay to ensure port is released
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Reset state before starting
    m_serverPort = 0;
    m_serverRunning = true;
    
    m_serverThread = std::thread([this]() {
        try {
            // Create Crow app
            m_app = std::make_unique<crow::SimpleApp>();
            
            // Serve static files from UI directory
            CROW_ROUTE((*m_app), "/")
            ([this]() {
                std::ifstream file(m_htmlPath);
                if (!file.is_open()) {
                    return crow::response(404, "text/plain", "File not found");
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                return crow::response(buffer.str());
            });
            
            // Serve CSS files
            CROW_ROUTE((*m_app), "/styles.css")
            ([this]() {
                std::string cssPath = m_uiDirectory + "/styles.css";
                std::ifstream file(cssPath);
                if (!file.is_open()) {
                    return crow::response(404, "text/plain", "CSS not found");
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                crow::response res(buffer.str());
                res.set_header("Content-Type", "text/css");
                return res;
            });
            
            // Serve JS files
            CROW_ROUTE((*m_app), "/app.js")
            ([this]() {
                std::string jsPath = m_uiDirectory + "/app.js";
                std::ifstream file(jsPath);
                if (!file.is_open()) {
                    return crow::response(404, "text/plain", "JS not found");
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                crow::response res(buffer.str());
                res.set_header("Content-Type", "application/javascript");
                return res;
            });
            
            // Serve catpack images
            CROW_ROUTE((*m_app), "/catpacks/<path>")
            ([](const crow::request& req, std::string path) {
                try {
                    // Sanitize the path to prevent directory traversal
                    std::filesystem::path filePath("catpacks");
                    filePath /= path;
                    
                    // Remove any ".." components and normalize
                    filePath = filePath.lexically_normal();
                    
                    // Ensure the path doesn't go outside catpacks directory
                    if (filePath.string().find("catpacks") != 0 && filePath.string().find("..") != std::string::npos) {
                        return crow::response(403, "text/plain", "Forbidden");
                    }
                    
                    // Check if file exists
                    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
                        return crow::response(404, "text/plain", "File not found");
                    }
                    
                    std::ifstream file(filePath, std::ios::binary);
                    if (!file.is_open()) {
                        return crow::response(404, "text/plain", "File not found");
                    }
                    
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    crow::response res(buffer.str());
                    
                    // Set content type based on file extension
                    std::string ext = filePath.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".png") {
                        res.set_header("Content-Type", "image/png");
                    } else if (ext == ".jpg" || ext == ".jpeg") {
                        res.set_header("Content-Type", "image/jpeg");
                    } else if (ext == ".gif") {
                        res.set_header("Content-Type", "image/gif");
                    } else {
                        res.set_header("Content-Type", "application/octet-stream");
                    }
                    
                    return res;
                } catch (const std::exception& e) {
                    return crow::response(500, "text/plain", "Internal server error");
                }
            });
            
            // Serve hat images
            CROW_ROUTE((*m_app), "/hats/<path>")
            ([](const crow::request& req, std::string path) {
                try {
                    // Sanitize the path to prevent directory traversal
                    std::filesystem::path filePath("hats");
                    filePath /= path;
                    
                    // Remove any ".." components and normalize
                    filePath = filePath.lexically_normal();
                    
                    // Ensure the path doesn't go outside hats directory
                    if (filePath.string().find("hats") != 0 && filePath.string().find("..") != std::string::npos) {
                        return crow::response(403, "text/plain", "Forbidden");
                    }
                    
                    // Check if file exists
                    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
                        return crow::response(404, "text/plain", "File not found");
                    }
                    
                    std::ifstream file(filePath, std::ios::binary);
                    if (!file.is_open()) {
                        return crow::response(404, "text/plain", "File not found");
                    }
                    
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    crow::response res(buffer.str());
                    
                    // Set content type based on file extension
                    std::string ext = filePath.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".png") {
                        res.set_header("Content-Type", "image/png");
                    } else if (ext == ".jpg" || ext == ".jpeg") {
                        res.set_header("Content-Type", "image/jpeg");
                    } else if (ext == ".gif") {
                        res.set_header("Content-Type", "image/gif");
                    } else {
                        res.set_header("Content-Type", "application/octet-stream");
                    }
                    
                    return res;
                } catch (const std::exception& e) {
                    return crow::response(500, "text/plain", "Internal server error");
                }
            });
            
            // API endpoint for messages from JavaScript
            CROW_ROUTE((*m_app), "/api/message").methods("POST"_method)
            ([this](const crow::request& req) {
                if (m_messageHandler) {
                    m_messageHandler(req.body);
                }
                return crow::response(200, "text/plain", "OK");
            });
            
            // API endpoint to send messages to JavaScript
            CROW_ROUTE((*m_app), "/api/send")
            ([this](const crow::request& req) {
                // This would be called from C++ to send messages to JS
                // For now, we use webview_eval instead
                return crow::response(200, "text/plain", "OK");
            });
            
            // Find an available port and run server
            int port = 18080;
            m_serverPort = port;
            
            // Run the server (this blocks, which is fine in a thread)
            // The run() call will block until stop() is called
            m_app->port(port).multithreaded().run();
        } catch (const std::exception& e) {
            std::cerr << "Crow server error: " << e.what() << std::endl;
            m_serverRunning = false;
            m_serverPort = 0;
        }
        
        // Thread is exiting - mark as not running
        m_serverRunning = false;
    });
}

void WebViewWindow::stopServer() {
    // Signal server to stop
    m_serverRunning = false;
    
    // Stop the Crow server if it exists
    if (m_app) {
        try {
            // Stop the Crow server properly
            m_app->stop();
        } catch (...) {
            // Ignore errors when stopping
        }
    }
    
    // Wait for the server thread to finish
    if (m_serverThread.joinable()) {
        // Give the server thread time to exit after stop() is called
        // The run() method should return after stop() is called
        auto startTime = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(2);
        
        // Wait for thread to finish, checking periodically
        while (m_serverThread.joinable() && 
               (std::chrono::steady_clock::now() - startTime) < timeout) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Now join the thread (this will block if thread is still running, but we've waited)
        if (m_serverThread.joinable()) {
            try {
                m_serverThread.join();
            } catch (...) {
                // If join fails, detach as last resort to avoid hanging
                m_serverThread.detach();
            }
        }
    }
    
    // Clean up resources only after thread has finished
    m_app.reset();
    m_serverPort = 0;
    
    // Ensure running flag is false
    m_serverRunning = false;
}

void WebViewWindow::navigateToFile(const std::string& htmlPath) {
    m_htmlPath = htmlPath;
    std::filesystem::path htmlFilePath(htmlPath);
    m_uiDirectory = htmlFilePath.parent_path().string();
    
    if (m_webview && m_serverPort > 0) {
        std::string url = "http://localhost:" + std::to_string(m_serverPort) + "/";
        webview_navigate(m_webview, url.c_str());
    }
}

void WebViewWindow::postMessage(const std::string& message) {
    if (!m_webview) {
        return;
    }
    
    // The message is already valid JSON, we need to escape it for embedding in JavaScript
    // Escape backslashes and quotes for embedding in JavaScript string literal
    std::string escaped;
    for (char c : message) {
        if (c == '\\') {
            escaped += "\\\\";
        } else if (c == '"') {
            escaped += "\\\"";
        } else if (c == '\n') {
            escaped += "\\n";
        } else if (c == '\r') {
            escaped += "\\r";
        } else if (c == '\t') {
            escaped += "\\t";
        } else {
            escaped += c;
        }
    }
    
    // Create copies of the scripts for the dispatch callback
    // We need to allocate memory that will persist until the callback executes
    std::string* script1Ptr = new std::string("try { var msg = JSON.parse(\"" + escaped + "\"); window.dispatchEvent(new MessageEvent('message', { data: msg })); } catch(e) { console.error('Error in postMessage:', e); }");
    std::string* script2Ptr = new std::string("try { if (typeof window.receiveMessage === 'function') { window.receiveMessage(\"" + escaped + "\"); } } catch(e) { console.error('Error in receiveMessage:', e); }");
    
    // Use webview_dispatch to safely call webview_eval from any thread
    // This ensures the evaluation happens on the main/GUI thread
    webview_dispatch(m_webview, evalScriptCallback, script1Ptr);
    webview_dispatch(m_webview, evalScriptCallback, script2Ptr);
}

void WebViewWindow::setMessageHandler(std::function<void(const std::string&)> handler) {
    m_messageHandler = handler;
}

void WebViewWindow::runMessageLoop() {
    if (m_webview) {
        // webview_run blocks until window is closed
        webview_run(m_webview);
    }
}

void WebViewWindow::shutdown() {
    // Make shutdown idempotent - if already shutting down, return early
    bool expected = false;
    if (!m_shuttingDown.compare_exchange_strong(expected, true)) {
        // Already shutting down, wait a bit for it to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return;
    }
    
    // Stop message handler first to prevent callbacks during shutdown
    m_messageHandler = nullptr;
    
    if (m_webview) {
        try {
            webview_destroy(m_webview);
        } catch (...) {
            // Ignore errors during destruction
        }
        m_webview = nullptr;
    }
    
    // Stop server and wait for it to fully stop
    stopServer();
    
    m_initialized = false;
}

void* WebViewWindow::getHwnd() const {
    if (m_webview) {
        return webview_get_window(m_webview);
    }
    return nullptr;
}

bool WebViewWindow::isWindowValid() const {
    if (!m_webview || !m_initialized) {
        return false;
    }
    
    #ifdef _WIN32
    HWND hwnd = static_cast<HWND>(getHwnd());
    if (hwnd) {
        return IsWindow(hwnd) != 0;
    }
    #endif
    
    // For non-Windows platforms, assume valid if initialized
    return true;
}

void WebViewWindow::hideWindow() {
    if (!m_webview || !m_initialized) {
        return;
    }
    
    #ifdef _WIN32
    HWND hwnd = static_cast<HWND>(getHwnd());
    if (hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }
    #endif
    // For non-Windows platforms, webview doesn't have a direct hide API
    // The window would need to be handled differently
}

void WebViewWindow::showWindow() {
    if (!m_webview || !m_initialized) {
        return;
    }
    
    #ifdef _WIN32
    HWND hwnd = static_cast<HWND>(getHwnd());
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        // Bring to front
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }
    #endif
    // For non-Windows platforms, webview doesn't have a direct show API
}
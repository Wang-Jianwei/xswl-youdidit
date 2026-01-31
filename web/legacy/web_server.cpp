// Backup of original src/web/web_server.cpp
// Moved to web/src/web_server.cpp during migration to web/ subproject.
// Kept here as legacy backup; do not use directly.

#include <xswl/youdidit/web/web_server.hpp>
#include <httplib.h>
#include <sstream>
#include <chrono>
#include <thread>

namespace xswl {
namespace youdidit {

WebServer::WebServer(WebDashboard *dashboard, int port)
    : dashboard_(dashboard), host_("0.0.0.0"), port_(port), running_(false), http_server_(nullptr) {}

WebServer::~WebServer() noexcept {
    stop();
}

void WebServer::start() {
    if (running_) return;
    
    try {
        // Cast void* to httplib::Server*
        httplib::Server *server = new httplib::Server();
        http_server_ = static_cast<void*>(server);
        
        // Setup routes BEFORE starting server
        _setup_routes();

        // Start listening in background thread
        // Official cpp-httplib: listen() is blocking, so run in thread
        running_ = true;
        server_thread_ = std::thread([this, server]() {
            server->listen(host_.c_str(), port_);
        });
        
        // Give the server a moment to bind
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } catch (...) {
        running_ = false;
        if (http_server_) {
            httplib::Server *server = static_cast<httplib::Server*>(http_server_);
            delete server;
            http_server_ = nullptr;
        }
    }
}

void WebServer::stop() {
    if (http_server_) {
        httplib::Server *server = static_cast<httplib::Server*>(http_server_);
        server->stop();
    }

    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    if (http_server_) {
        httplib::Server *server = static_cast<httplib::Server*>(http_server_);
        delete server;
        http_server_ = nullptr;
    }
}

} // namespace youdidit
} // namespace xswl

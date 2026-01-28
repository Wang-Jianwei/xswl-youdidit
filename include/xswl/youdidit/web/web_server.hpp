#ifndef XSWL_YOUDIDIT_WEB_WEB_SERVER_HPP
#define XSWL_YOUDIDIT_WEB_WEB_SERVER_HPP

#include <xswl/youdidit/web/web_dashboard.hpp>
#include <atomic>
#include <memory>
#include <thread>

namespace xswl {
namespace youdidit {

class WebServer {
public:
    WebServer(WebDashboard *dashboard, int port = 8080);
    ~WebServer() noexcept;

    void start();
    void stop();
    bool is_running() const noexcept;

    void set_port(int port);
    int get_port() const noexcept;
    
    void set_host(const std::string &host);
    const std::string &host() const noexcept;

private:
    WebDashboard *dashboard_;
    std::string host_;
    int port_;
    std::atomic<bool> running_;
    void *http_server_;  // Opaque pointer to httplib::Server
    std::thread server_thread_;
    
    // Private helper methods
    void _setup_routes();
    std::string _get_dashboard_html();
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_WEB_WEB_SERVER_HPP

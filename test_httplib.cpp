#include "third_party/httplib.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("Hello World!", "text/plain");
    });

    svr.Get("/hi", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("Hi there!", "text/plain");
    });

    std::cout << "Starting server on http://0.0.0.0:9999 ..." << std::endl;
    
    std::thread t([&svr]() {
        svr.listen("0.0.0.0", 9999);
    });

    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "Server started. Press Ctrl+C to stop." << std::endl;
    
    t.join();
    return 0;
}

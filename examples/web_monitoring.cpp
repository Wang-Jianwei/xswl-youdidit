#include <xswl/youdidit/youdidit.hpp>
#include <iostream>
#include <chrono>

using namespace xswl::youdidit;

int main() {
    TaskPlatform platform;
    EventLog event_log;

    auto claimer = std::make_shared<Claimer>("monitor", "MonitorWorker");
    claimer->add_category("monitoring");
    platform.register_claimer(claimer);

    auto builder = platform.task_builder();
    builder.title("Generate Metrics")
           .description("simulate metric producing job")
           .priority(65)
           .category("monitoring")
           .handler([](Task & /*task*/, const std::string & /*input*/) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "metrics collected"};
           });
    auto task = builder.build();
    platform.publish_task(task);
    claimer->claim_task(task->id());
    claimer->run_task(task->id(), "collect");

    event_log.add_event(EventLog::EventType::Info, "web", "dashboard started");

    MetricsExporter exporter(&platform, &event_log);
    std::cout << "Metrics (JSON): " << exporter.export_json() << std::endl;
    std::cout << "Metrics (Prometheus):\n" << exporter.export_prometheus() << std::endl;

    WebDashboard dashboard(&platform);
    dashboard.start_server(8088);
    std::cout << "Dashboard running on port 8088: " << (dashboard.is_running() ? "yes" : "no") << std::endl;

    WebServer server(&dashboard, 8088);
    server.start();
    std::cout << "WebServer running: " << (server.is_running() ? "yes" : "no") << std::endl;
    server.stop();

    dashboard.stop_server();
    std::cout << "Dashboard stopped: " << (!dashboard.is_running() ? "yes" : "no") << std::endl;
    return 0;
}

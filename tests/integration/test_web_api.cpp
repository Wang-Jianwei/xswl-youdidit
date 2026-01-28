#include <xswl/youdidit/youdidit.hpp>
#include <iostream>
#include <chrono>

using namespace xswl::youdidit;

namespace {
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " << msg << std::endl; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "Running " << #fn << "... "; \
        if (fn()) { \
            std::cout << "PASSED" << std::endl; \
        } else { \
            std::cout << "FAILED" << std::endl; \
            all_passed = false; \
        } \
    } while (0)

std::shared_ptr<Task> build_and_publish(TaskPlatform &platform,
                                        const std::string &title,
                                        const std::string &category) {
    auto builder = platform.task_builder();
    builder.title(title)
           .description("web api task")
           .priority(55)
           .category(category)
           .handler([](Task & /*task*/, const std::string &input) -> tl::expected<TaskResult, std::string> {
               TaskResult r(true, "ok" + input);
               r.output_data["len"] = std::to_string(input.size());
               return r;
           });
    auto task = builder.build();
    platform.publish_task(task);
    return task;
}

bool test_metrics_dashboard_and_server() {
    TaskPlatform platform;
    EventLog event_log;

    auto claimer = std::make_shared<Claimer>("claimer-web", "Charlie");
    claimer->add_category("web");
    platform.register_claimer(claimer);

    auto task = build_and_publish(platform, "Web Metrics", "web");
    auto claim_res = claimer->claim_task(task->id());
    TEST_ASSERT(claim_res.has_value(), "Claim should succeed");

    auto exec_res = claimer->run_task(task->id(), "payload");
    TEST_ASSERT(exec_res.has_value(), "Execution should succeed");
    event_log.add_event(EventLog::EventType::Info, "integration", "task executed");

    MetricsExporter exporter(&platform, &event_log);
    auto json = exporter.export_json();
    auto prom = exporter.export_prometheus();
    TEST_ASSERT(json.find("\"total_tasks\"") != std::string::npos, "JSON should contain total_tasks");
    TEST_ASSERT(prom.find("youdidit_events_total") != std::string::npos, "Prometheus output should expose events");

    WebDashboard dashboard(&platform);
    dashboard.start_server(9090);
    TEST_ASSERT(dashboard.is_running(), "Dashboard should start");

    auto summaries = dashboard.get_tasks_summary();
    TEST_ASSERT(!summaries.empty(), "Dashboard should list tasks");
    auto logs = dashboard.get_event_logs();
    TEST_ASSERT(logs.empty(), "Dashboard uses its own event log and starts empty");

    auto perf = dashboard.analyze_performance(std::chrono::system_clock::now() - std::chrono::minutes(1),
                                              std::chrono::system_clock::now());
    TEST_ASSERT(perf.total_tasks == platform.get_statistics().total_tasks, "Performance should reflect platform stats");

    WebServer server(&dashboard, 9091);
    server.start();
    TEST_ASSERT(server.is_running(), "Server should start");
    server.stop();
    TEST_ASSERT(!server.is_running(), "Server should stop");

    dashboard.stop_server();
    TEST_ASSERT(!dashboard.is_running(), "Dashboard should stop");

    return true;
}

} // namespace

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      Integration Web API Tests        " << std::endl;
    std::cout << "========================================" << std::endl;

    bool all_passed = true;
    RUN_TEST(test_metrics_dashboard_and_server);

    std::cout << "========================================" << std::endl;
    if (all_passed) {
        std::cout << "  ✓ All integration web tests PASSED" << std::endl;
    } else {
        std::cout << "  ✗ Some integration web tests FAILED" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return all_passed ? 0 : 1;
}

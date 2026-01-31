#include <xswl/youdidit/web/event_log.hpp>
#include <xswl/youdidit/web/time_replay.hpp>
#include <xswl/youdidit/web/metrics_exporter.hpp>
#include <xswl/youdidit/web/web_dashboard.hpp>
#include <xswl/youdidit/web/web_server.hpp>
#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task_builder.hpp>
#include <cassert>
#include <iostream>
#include <thread>
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

Timestamp make_time_shift_ms(int delta_ms) {
    return std::chrono::system_clock::now() + std::chrono::milliseconds(delta_ms);
}

std::shared_ptr<Task> make_task_with_status(TaskPlatform &platform, const std::string &id, TaskStatus status) {
    auto task = std::make_shared<Task>(id);
    task->set_status(TaskStatus::Published);
    if (status == TaskStatus::Claimed || status == TaskStatus::Processing || status == TaskStatus::Completed || status == TaskStatus::Failed) {
        task->set_status(TaskStatus::Claimed);
    }
    if (status == TaskStatus::Processing || status == TaskStatus::Completed || status == TaskStatus::Failed) {
        task->set_status(TaskStatus::Processing);
    }
    if (status == TaskStatus::Completed) {
        task->set_status(TaskStatus::Completed);
    }
    if (status == TaskStatus::Failed) {
        task->set_status(TaskStatus::Failed);
    }
    platform.publish_task(task);
    return task;
}

bool test_event_log_basic() {
    EventLog log;
    log.add_event(EventLog::EventType::Info, "sourceA", "message1");
    log.add_event(EventLog::EventType::Warning, "sourceB", "warn");
    log.add_event(EventLog::EventType::Error, "sourceA", "error");

    EventLog::Filter f;
    f.source = std::string("sourceA");
    auto events_source = log.get_events(f);
    TEST_ASSERT(events_source.size() == 2, "Filter by source should return 2 events");

    f = EventLog::Filter{};
    f.type = EventLog::EventType::Warning;
    auto warnings = log.get_events(f);
    TEST_ASSERT(warnings.size() == 1, "Filter by type should return 1 warning");

    auto before_cleanup = log.size();
    log.cleanup_events_before(make_time_shift_ms(-1));
    TEST_ASSERT(log.size() == before_cleanup, "Cleanup with past time should keep events");

    log.cleanup_events_before(make_time_shift_ms(1000));
    TEST_ASSERT(log.size() == 0, "Cleanup with far future should remove all");

    return true;
}

bool test_time_replay_snapshot() {
    TaskPlatform platform;
    EventLog log;

    auto t1 = make_task_with_status(platform, "t1", TaskStatus::Completed);
    auto t2 = make_task_with_status(platform, "t2", TaskStatus::Failed);
    (void)t1; (void)t2;

    log.add_event_at(make_time_shift_ms(-10), EventLog::EventType::Info, "src", "past");
    log.add_event_at(make_time_shift_ms(10), EventLog::EventType::Info, "src", "future");

    TimeReplay replay(&log, &platform);
    auto snap = replay.snapshot_at(make_time_shift_ms(0));

    TEST_ASSERT(snap.total_tasks == 2, "Snapshot should count tasks");
    TEST_ASSERT(snap.completed_tasks == 1, "Snapshot should count completed tasks");
    TEST_ASSERT(snap.failed_tasks == 1, "Snapshot should count failed tasks");
    TEST_ASSERT(snap.events_count == 1, "Snapshot should include past event only");

    auto between = replay.events_between(make_time_shift_ms(-20), make_time_shift_ms(5));
    TEST_ASSERT(between.size() == 1, "Events between should capture one event");

    return true;
}

bool test_metrics_exporter_formats() {
    TaskPlatform platform;
    EventLog log;
    make_task_with_status(platform, "t1", TaskStatus::Completed);
    log.add_event(EventLog::EventType::Info, "src", "msg");

    MetricsExporter exporter(&platform, &log);
    auto json = exporter.export_json();
    auto prom = exporter.export_prometheus();

    TEST_ASSERT(json.find("total_tasks") != std::string::npos, "JSON should contain total_tasks");
    TEST_ASSERT(prom.find("youdidit_tasks_total") != std::string::npos, "Prometheus should contain tasks metric");
    TEST_ASSERT(prom.find("youdidit_events_total") != std::string::npos, "Prometheus should contain events metric");

    return true;
}

bool test_web_dashboard_summaries() {
    TaskPlatform platform;
    auto task = std::make_shared<Task>("t1");
    task->set_status(TaskStatus::Published);
    platform.publish_task(task);

    auto claimer = std::make_shared<Claimer>("c1", "claimer");
    platform.register_claimer(claimer);

    WebDashboard dashboard(&platform);
    auto metrics = dashboard.get_metrics();
    TEST_ASSERT(metrics.total_tasks == 1, "Metrics should see 1 task");

    auto tasks = dashboard.get_tasks_summary();
    TEST_ASSERT(tasks.size() == 1, "Task summary size should be 1");

    auto claimers = dashboard.get_claimers_summary();
    TEST_ASSERT(claimers.size() == 1, "Claimer summary size should be 1");

    dashboard.start_server(8080);
    TEST_ASSERT(dashboard.is_running(), "Dashboard should be running after start");
    dashboard.stop_server();
    TEST_ASSERT(!dashboard.is_running(), "Dashboard should stop");

    auto logs = dashboard.get_event_logs();
    TEST_ASSERT(logs.empty(), "Event log should be empty by default");

    auto perf = dashboard.analyze_performance(make_time_shift_ms(-1000), make_time_shift_ms(0));
    TEST_ASSERT(perf.total_tasks == 1, "Performance analysis should reflect tasks");

    auto json = dashboard.export_as_json(make_time_shift_ms(-1000), make_time_shift_ms(0));
    TEST_ASSERT(json.find("total_tasks") != std::string::npos, "Dashboard JSON should contain metrics");

    return true;
}

bool test_web_server_start_stop() {
    TaskPlatform platform;
    WebDashboard dashboard(&platform);
    WebServer server(&dashboard, 8081);
    server.start();
    TEST_ASSERT(server.is_running(), "Server should be running after start");
    server.stop();
    TEST_ASSERT(!server.is_running(), "Server should stop");
    return true;
}

} // namespace

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "          Web Module Unit Tests         " << std::endl;
    std::cout << "========================================" << std::endl;

    bool all_passed = true;

    RUN_TEST(test_event_log_basic);
    RUN_TEST(test_time_replay_snapshot);
    RUN_TEST(test_metrics_exporter_formats);
    RUN_TEST(test_web_dashboard_summaries);
    RUN_TEST(test_web_server_start_stop);

    std::cout << "========================================" << std::endl;
    if (all_passed) {
        std::cout << "  ✓ All tests PASSED" << std::endl;
    } else {
        std::cout << "  ✗ Some tests FAILED" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return all_passed ? 0 : 1;
}
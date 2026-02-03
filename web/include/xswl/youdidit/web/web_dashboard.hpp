#ifndef XSWL_YOUDIDIT_WEB_WEB_DASHBOARD_HPP
#define XSWL_YOUDIDIT_WEB_WEB_DASHBOARD_HPP

#include <xswl/youdidit/web/event_log.hpp>
#include <xswl/youdidit/web/time_replay.hpp>
#include <xswl/youdidit/web/metrics_exporter.hpp>
#include <xswl/youdidit/core/claimer.hpp>
#include <xswl/youdidit/core/task_platform.hpp>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace xswl {
namespace youdidit {

class WebDashboard {
public:
    // 同进程模式
    explicit WebDashboard(TaskPlatform *platform);

    // 远程模式（占位实现）
    explicit WebDashboard(const std::string &metrics_endpoint);

    // 多平台聚合模式（占位实现）
    explicit WebDashboard(const std::vector<std::string> &endpoints);

    ~WebDashboard();

    void start_server(int port = 8080);
    void stop_server();
    bool is_running() const noexcept;

    WebDashboard &set_update_interval(int milliseconds);
    WebDashboard &set_log_file_path(const std::string &path);
    WebDashboard &set_max_event_history(size_t max_events);
    WebDashboard &enable_https(const std::string &cert_path, const std::string &key_path);

    using DashboardMetrics = TaskPlatform::PlatformStatistics;
    DashboardMetrics get_metrics() const;
    std::string get_dashboard_data() const;

    struct TaskSummary {
        TaskId id;
        std::string title;
        std::string category;
        int priority;
        TaskStatus status;
        Timestamp published_at;
        std::string claimer_id;
    };

    struct ClaimerSummary {
        std::string id;
        std::string name;
        ClaimerState status;
        int claimed_task_count;
        std::uint64_t total_completed;
        std::uint64_t total_failed;
    }; 

    std::vector<TaskSummary> get_tasks_summary() const;
    std::vector<ClaimerSummary> get_claimers_summary() const;

    std::shared_ptr<TimeReplay> get_time_replay() const;
    std::vector<std::string> get_event_logs(int limit = 100, int offset = 0) const;

    struct PerformanceAnalysis {
        double tasks_per_minute;
        double completion_rate;
        double failure_rate;
        size_t total_tasks;
    };

    PerformanceAnalysis analyze_performance(const Timestamp &start_time, const Timestamp &end_time) const;

    std::string export_as_json(const Timestamp &start, const Timestamp &end) const;
    std::string export_as_csv(const Timestamp &start, const Timestamp &end) const;

private:
    DashboardMetrics _empty_metrics() const;
    std::string _serialize_metrics_json(const DashboardMetrics &metrics) const;

    TaskPlatform *platform_;
    EventLog *event_log_;
    std::unique_ptr<EventLog> owned_event_log_;
    std::shared_ptr<TimeReplay> time_replay_;

    std::vector<std::string> endpoints_;
    int update_interval_ms_;
    size_t max_event_history_;
    std::string log_file_path_;
    bool running_;
    int port_;
    bool https_enabled_;
    std::string cert_path_;
    std::string key_path_;
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_WEB_WEB_DASHBOARD_HPP

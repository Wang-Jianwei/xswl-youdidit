#include <xswl/youdidit/web/web_dashboard.hpp>
#include <algorithm>
#include <chrono>
#include <sstream>

namespace xswl {
namespace youdidit {

namespace {
TaskPlatform::PlatformStatistics make_zero_stats() {
    TaskPlatform::PlatformStatistics stats{};
    stats.start_time = std::chrono::system_clock::now();
    stats.total_tasks = 0;
    stats.published_tasks = 0;
    stats.claimed_tasks = 0;
    stats.processing_tasks = 0;
    stats.completed_tasks = 0;
    stats.failed_tasks = 0;
    stats.abandoned_tasks = 0;
    stats.total_claimers = 0;
    return stats;
}
}

WebDashboard::WebDashboard(TaskPlatform *platform)
    : platform_(platform),
      event_log_(nullptr),
      time_replay_(nullptr),
      update_interval_ms_(1000),
      max_event_history_(1000),
      running_(false),
      port_(0),
      https_enabled_(false) {
    owned_event_log_.reset(new EventLog());
    event_log_ = owned_event_log_.get();
    time_replay_ = std::make_shared<TimeReplay>(event_log_, platform_);
}

WebDashboard::WebDashboard(const std::string &metrics_endpoint)
    : WebDashboard(static_cast<TaskPlatform *>(nullptr)) {
    endpoints_.push_back(metrics_endpoint);
}

WebDashboard::WebDashboard(const std::vector<std::string> &endpoints)
    : WebDashboard(static_cast<TaskPlatform *>(nullptr)) {
    endpoints_ = endpoints;
}

WebDashboard::~WebDashboard() = default;

void WebDashboard::start_server(int port) {
    port_ = port;
    running_ = true;
}

void WebDashboard::stop_server() {
    running_ = false;
}

bool WebDashboard::is_running() const noexcept {
    return running_;
}

WebDashboard &WebDashboard::set_update_interval(int milliseconds) {
    update_interval_ms_ = milliseconds;
    return *this;
}

WebDashboard &WebDashboard::set_log_file_path(const std::string &path) {
    log_file_path_ = path;
    return *this;
}

WebDashboard &WebDashboard::set_max_event_history(size_t max_events) {
    max_event_history_ = max_events;
    return *this;
}

WebDashboard &WebDashboard::enable_https(const std::string &cert_path, const std::string &key_path) {
    https_enabled_ = true;
    cert_path_ = cert_path;
    key_path_ = key_path;
    return *this;
}

WebDashboard::DashboardMetrics WebDashboard::get_metrics() const {
    if (platform_) {
        return platform_->get_statistics();
    }
    return _empty_metrics();
}

std::string WebDashboard::get_dashboard_data() const {
    MetricsExporter exporter(platform_, event_log_);
    return exporter.export_json();
}

std::vector<WebDashboard::TaskSummary> WebDashboard::get_tasks_summary() const {
    std::vector<TaskSummary> summaries;
    if (!platform_) {
        return summaries;
    }
    auto tasks = platform_->get_tasks();
    summaries.reserve(tasks.size());
    for (const auto &task : tasks) {
        TaskSummary summary;
        summary.id = task->id();
        summary.title = task->title();
        summary.category = task->category();
        summary.priority = task->priority();
        summary.status = task->status();
        summary.published_at = task->published_at();
        summary.claimer_id = task->claimer_id();
        summaries.push_back(summary);
    }
    return summaries;
}

std::vector<WebDashboard::ClaimerSummary> WebDashboard::get_claimers_summary() const {
    std::vector<ClaimerSummary> summaries;
    if (!platform_) {
        return summaries;
    }
    auto claimers = platform_->get_claimers();
    summaries.reserve(claimers.size());
    for (const auto &claimer : claimers) {
        ClaimerSummary summary;
        summary.id = claimer->id();
        summary.name = claimer->name();
        summary.status = claimer->status();
        summary.active_task_count = claimer->active_task_count();
        summary.total_completed = claimer->total_completed();
        summary.total_failed = claimer->total_failed();
        summaries.push_back(summary);
    }
    return summaries;
}

std::shared_ptr<TimeReplay> WebDashboard::get_time_replay() const {
    return time_replay_;
}

std::vector<std::string> WebDashboard::get_event_logs(int limit, int offset) const {
    std::vector<std::string> logs;
    if (!event_log_) {
        return logs;
    }
    auto events = event_log_->get_events();
    if (offset < 0) {
        offset = 0;
    }
    size_t begin = static_cast<size_t>(offset);
    if (begin >= events.size()) {
        return logs;
    }
    size_t end = std::min(begin + static_cast<size_t>(limit), events.size());
    logs.reserve(end - begin);
    for (size_t i = begin; i < end; ++i) {
        const auto &ev = events[i];
        std::ostringstream oss;
        oss << ev.timestamp.time_since_epoch().count() << "|" << ev.source << "|" << ev.message;
        logs.push_back(oss.str());
    }
    return logs;
}

WebDashboard::PerformanceAnalysis WebDashboard::analyze_performance(const Timestamp &start_time, const Timestamp &end_time) const {
    PerformanceAnalysis analysis{};
    auto metrics = get_metrics();
    analysis.total_tasks = metrics.total_tasks;

    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    if (duration_ms <= 0) {
        duration_ms = 1;
    }
    double duration_minutes = static_cast<double>(duration_ms) / 60000.0;
    double finished = static_cast<double>(metrics.completed_tasks + metrics.failed_tasks);

    analysis.tasks_per_minute = finished / duration_minutes;
    if (finished > 0.0) {
        analysis.completion_rate = static_cast<double>(metrics.completed_tasks) / finished;
        analysis.failure_rate = static_cast<double>(metrics.failed_tasks) / finished;
    } else {
        analysis.completion_rate = 0.0;
        analysis.failure_rate = 0.0;
    }

    return analysis;
}

std::string WebDashboard::export_as_json(const Timestamp &start, const Timestamp &end) const {
    auto metrics = get_metrics();
    std::ostringstream oss;
    oss << "{";
    oss << _serialize_metrics_json(metrics);

    if (event_log_) {
        EventLog::Filter f;
        f.start_time = start;
        f.end_time = end;
        auto events = event_log_->get_events(f);
        oss << ",\"events_in_range\":" << events.size();
    }
    oss << "}";
    return oss.str();
}

std::string WebDashboard::export_as_csv(const Timestamp &start, const Timestamp &end) const {
    (void)start;
    (void)end;
    auto metrics = get_metrics();
    std::ostringstream oss;
    oss << "metric,value\n";
    oss << "total_tasks," << metrics.total_tasks << "\n";
    oss << "completed_tasks," << metrics.completed_tasks << "\n";
    oss << "failed_tasks," << metrics.failed_tasks << "\n";
    oss << "published_tasks," << metrics.published_tasks << "\n";
    oss << "claimed_tasks," << metrics.claimed_tasks << "\n";
    oss << "processing_tasks," << metrics.processing_tasks << "\n";
    oss << "abandoned_tasks," << metrics.abandoned_tasks << "\n";
    oss << "total_claimers," << metrics.total_claimers << "\n";
    if (event_log_) {
        oss << "events," << event_log_->size() << "\n";
    }
    return oss.str();
}

WebDashboard::DashboardMetrics WebDashboard::_empty_metrics() const {
    return make_zero_stats();
}

std::string WebDashboard::_serialize_metrics_json(const DashboardMetrics &metrics) const {
    std::ostringstream oss;
    oss << "\"total_tasks\":" << metrics.total_tasks << ",";
    oss << "\"completed_tasks\":" << metrics.completed_tasks << ",";
    oss << "\"failed_tasks\":" << metrics.failed_tasks << ",";
    oss << "\"published_tasks\":" << metrics.published_tasks << ",";
    oss << "\"claimed_tasks\":" << metrics.claimed_tasks << ",";
    oss << "\"processing_tasks\":" << metrics.processing_tasks << ",";
    oss << "\"abandoned_tasks\":" << metrics.abandoned_tasks << ",";
    oss << "\"total_claimers\":" << metrics.total_claimers;
    return oss.str();
}

} // namespace youdidit
} // namespace xswl

// Backup of original src/web/metrics_exporter.cpp
// Moved to web/src/metrics_exporter.cpp during migration to web/ subproject.
// Kept here as legacy backup; do not use directly.

#include <xswl/youdidit/web/metrics_exporter.hpp>
#include <sstream>

namespace xswl {
namespace youdidit {

MetricsExporter::MetricsExporter(TaskPlatform *platform, EventLog *event_log)
    : platform_(platform), event_log_(event_log) {}

std::string MetricsExporter::export_json() const {
    std::ostringstream oss;
    oss << "{";
    if (platform_) {
        auto stats = platform_->get_statistics();
        oss << "\"total_tasks\":" << stats.total_tasks << ",";
        oss << "\"completed_tasks\":" << stats.completed_tasks << ",";
        oss << "\"failed_tasks\":" << stats.failed_tasks << ",";
        oss << "\"published_tasks\":" << stats.published_tasks << ",";
        oss << "\"claimed_tasks\":" << stats.claimed_tasks << ",";
        oss << "\"processing_tasks\":" << stats.processing_tasks << ",";
        oss << "\"abandoned_tasks\":" << stats.abandoned_tasks << ",";
        oss << "\"total_claimers\":" << stats.total_claimers;
    }
    if (event_log_) {
        if (platform_) {
            oss << ",";
        }
        oss << "\"events\":" << event_log_->size();
    }
    oss << "}";
    return oss.str();
}

std::string MetricsExporter::export_prometheus() const {
    std::ostringstream oss;
    if (platform_) {
        auto stats = platform_->get_statistics();
        oss << "youdidit_tasks_total " << stats.total_tasks << "\n";
        oss << "youdidit_tasks_completed " << stats.completed_tasks << "\n";
        oss << "youdidit_tasks_failed " << stats.failed_tasks << "\n";
        oss << "youdidit_tasks_published " << stats.published_tasks << "\n";
        oss << "youdidit_tasks_claimed " << stats.claimed_tasks << "\n";
        oss << "youdidit_tasks_processing " << stats.processing_tasks << "\n";
        oss << "youdidit_tasks_abandoned " << stats.abandoned_tasks << "\n";
        oss << "youdidit_claimers_total " << stats.total_claimers << "\n";
    }
    if (event_log_) {
        oss << "youdidit_events_total " << event_log_->size() << "\n";
    }
    return oss.str();
}

} // namespace youdidit
} // namespace xswl

// Backup of original src/web/web_dashboard.cpp
// Moved to web/src/web_dashboard.cpp during migration to web/ subproject.
// Kept here as legacy backup; do not use directly.

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

// ... (legacy backup content truncated) ...

} // namespace youdidit
} // namespace xswl

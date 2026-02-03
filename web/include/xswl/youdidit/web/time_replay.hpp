#ifndef XSWL_YOUDIDIT_WEB_TIME_REPLAY_HPP
#define XSWL_YOUDIDIT_WEB_TIME_REPLAY_HPP

#include <xswl/youdidit/web/event_log.hpp>
#include <xswl/youdidit/core/task_platform.hpp>
#include <vector>
#include <chrono>

namespace xswl {
namespace youdidit {

class TimeReplay {
public:
    struct Snapshot {
        Timestamp at;
        size_t total_tasks{0};
        size_t completed_tasks{0};
        size_t failed_tasks{0};
        size_t events_count{0};
    };

    explicit TimeReplay(EventLog *event_log, TaskPlatform *platform);

    Snapshot snapshot_at(const Timestamp &at) const;

    std::vector<EventLog::Event> events_between(const Timestamp &start, const Timestamp &end) const;

private:
    EventLog *event_log_;
    TaskPlatform *platform_;
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_WEB_TIME_REPLAY_HPP

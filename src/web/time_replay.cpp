/* DEPRECATED: Moved to `web/src/time_replay.cpp`. See web/legacy/ for backup. */


namespace xswl {
namespace youdidit {

TimeReplay::TimeReplay(EventLog *event_log, TaskPlatform *platform)
    : event_log_(event_log), platform_(platform) {}

TimeReplay::Snapshot TimeReplay::snapshot_at(const Timestamp &at) const {
    Snapshot snap{};
    snap.at = at;
    if (platform_) {
        auto stats = platform_->get_statistics();
        snap.total_tasks = stats.total_tasks;
        snap.completed_tasks = stats.completed_tasks;
        snap.failed_tasks = stats.failed_tasks;
    }
    if (event_log_) {
        EventLog::Filter f;
        f.end_time = at;
        snap.events_count = event_log_->get_events(f).size();
    }
    return snap;
}

std::vector<EventLog::Event> TimeReplay::events_between(const Timestamp &start, const Timestamp &end) const {
    if (!event_log_) {
        return {};
    }
    EventLog::Filter f;
    f.start_time = start;
    f.end_time = end;
    return event_log_->get_events(f);
}

} // namespace youdidit
} // namespace xswl

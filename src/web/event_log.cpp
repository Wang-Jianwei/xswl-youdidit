/* DEPRECATED: Moved to `web/src/event_log.cpp`. See web/legacy/ for backup. */


namespace xswl {
namespace youdidit {

EventLog::EventLog() = default;

void EventLog::add_event(EventType type,
                         const std::string &source,
                         const std::string &message,
                         const std::map<std::string, std::string> &metadata) {
    add_event_at(std::chrono::system_clock::now(), type, source, message, metadata);
}

void EventLog::add_event_at(const Timestamp &ts,
                            EventType type,
                            const std::string &source,
                            const std::string &message,
                            const std::map<std::string, std::string> &metadata) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(Event{ts, type, source, message, metadata});
}

std::vector<EventLog::Event> EventLog::get_events(const Filter &filter) const {
    std::vector<Event> result;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &ev : events_) {
        if (filter.type.has_value() && ev.type != filter.type.value()) {
            continue;
        }
        if (filter.source.has_value() && ev.source != filter.source.value()) {
            continue;
        }
        if (filter.start_time.has_value() && ev.timestamp < filter.start_time.value()) {
            continue;
        }
        if (filter.end_time.has_value() && ev.timestamp > filter.end_time.value()) {
            continue;
        }
        result.push_back(ev);
    }
    return result;
}

void EventLog::cleanup_events_before(const Timestamp &before_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.erase(std::remove_if(events_.begin(), events_.end(), [&](const Event &ev) {
        return ev.timestamp < before_time;
    }), events_.end());
}

size_t EventLog::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
}

} // namespace youdidit
} // namespace xswl

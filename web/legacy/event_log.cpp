// Backup of original src/web/event_log.cpp
// Moved to web/src/event_log.cpp during migration to web/ subproject.
// Kept here as legacy backup; do not use directly.

#include <xswl/youdidit/web/event_log.hpp>
#include <algorithm>

namespace xswl {
namespace youdidit {

EventLog::EventLog() = default;

void EventLog::add_event(EventType type,
                         const std::string &source,
                         const std::string &message,
                         const std::map<std::string, std::string> &metadata) {
    add_event_at(std::chrono::system_clock::now(), type, source, message, metadata);
}

// ... (legacy backup content truncated) ...

} // namespace youdidit
} // namespace xswl

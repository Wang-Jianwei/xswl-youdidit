#ifndef XSWL_YOUDIDIT_WEB_EVENT_LOG_HPP
#define XSWL_YOUDIDIT_WEB_EVENT_LOG_HPP

#include <xswl/youdidit/core/types.hpp>
#include <tl/optional.hpp>
#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace xswl {
namespace youdidit {

class EventLog {
public:
    enum class EventType {
        Info,
        Warning,
        Error,
        Task
    };

    struct Event {
        Timestamp timestamp;
        EventType type;
        std::string source;
        std::string message;
        std::map<std::string, std::string> metadata;
    };

    struct Filter {
        tl::optional<EventType> type;
        tl::optional<std::string> source;
        tl::optional<Timestamp> start_time;
        tl::optional<Timestamp> end_time;
    };

    EventLog();

    void add_event(EventType type,
                   const std::string &source,
                   const std::string &message,
                   const std::map<std::string, std::string> &metadata = {});

    void add_event_at(const Timestamp &ts,
                      EventType type,
                      const std::string &source,
                      const std::string &message,
                      const std::map<std::string, std::string> &metadata = {});

    std::vector<Event> get_events(const Filter &filter = {}) const;

    void cleanup_events_before(const Timestamp &before_time);

    size_t size() const;

private:
    std::vector<Event> events_;
    mutable std::mutex mutex_;
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_WEB_EVENT_LOG_HPP

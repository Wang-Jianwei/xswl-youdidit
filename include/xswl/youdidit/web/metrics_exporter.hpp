#ifndef XSWL_YOUDIDIT_WEB_METRICS_EXPORTER_HPP
#define XSWL_YOUDIDIT_WEB_METRICS_EXPORTER_HPP

#include <xswl/youdidit/web/event_log.hpp>
#include <xswl/youdidit/core/task_platform.hpp>
#include <string>

namespace xswl {
namespace youdidit {

class MetricsExporter {
public:
    MetricsExporter(TaskPlatform *platform, EventLog *event_log);

    std::string export_json() const;
    std::string export_prometheus() const;

private:
    TaskPlatform *platform_;
    EventLog *event_log_;
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_WEB_METRICS_EXPORTER_HPP

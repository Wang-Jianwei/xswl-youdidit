/**
 * @file types.cpp
 * @brief 核心类型实现
 */

#include <xswl/youdidit/core/types.hpp>
#include <map>

namespace xswl {
namespace youdidit {

// ========== TaskStatus 相关实现 ==========

std::string to_string(TaskStatus status) {
    static const std::map<TaskStatus, std::string> status_map = {
        {TaskStatus::Draft, "Draft"},
        {TaskStatus::Published, "Published"},
        {TaskStatus::Claimed, "Claimed"},
        {TaskStatus::Processing, "Processing"},
        {TaskStatus::Paused, "Paused"},
        {TaskStatus::Completed, "Completed"},
        {TaskStatus::Failed, "Failed"},
        {TaskStatus::Cancelled, "Cancelled"},
        {TaskStatus::Abandoned, "Abandoned"}
    };
    
    auto it = status_map.find(status);
    if (it != status_map.end()) {
        return it->second;
    }
    return "Unknown";
}

tl::optional<TaskStatus> task_status_from_string(const std::string &str) {
    static const std::map<std::string, TaskStatus> string_map = {
        {"Draft", TaskStatus::Draft},
        {"Published", TaskStatus::Published},
        {"Claimed", TaskStatus::Claimed},
        {"Processing", TaskStatus::Processing},
        {"Paused", TaskStatus::Paused},
        {"Completed", TaskStatus::Completed},
        {"Failed", TaskStatus::Failed},
        {"Cancelled", TaskStatus::Cancelled},
        {"Abandoned", TaskStatus::Abandoned}
    };
    
    auto it = string_map.find(str);
    if (it != string_map.end()) {
        return it->second;
    }
    return tl::nullopt;
}

// ========== ClaimerStatus 相关实现 ==========

std::string to_string(ClaimerStatus status) {
    static const std::map<ClaimerStatus, std::string> status_map = {
        {ClaimerStatus::Idle, "Idle"},
        {ClaimerStatus::Busy, "Busy"},
        {ClaimerStatus::Offline, "Offline"},
        {ClaimerStatus::Paused, "Paused"}
    };
    
    auto it = status_map.find(status);
    if (it != status_map.end()) {
        return it->second;
    }
    return "Unknown";
}

tl::optional<ClaimerStatus> claimer_status_from_string(const std::string &str) {
    static const std::map<std::string, ClaimerStatus> string_map = {
        {"Idle", ClaimerStatus::Idle},
        {"Busy", ClaimerStatus::Busy},
        {"Offline", ClaimerStatus::Offline},
        {"Paused", ClaimerStatus::Paused}
    };
    
    auto it = string_map.find(str);
    if (it != string_map.end()) {
        return it->second;
    }
    return tl::nullopt;
}

// ========== TaskResult 实现 ==========

TaskResult::TaskResult() 
    : success(false), summary("") {
}

TaskResult::TaskResult(bool success, const std::string &summary)
    : success(success), summary(summary) {
}

// ========== Error 实现 ==========

Error::Error(const std::string &msg, ErrorCode error_code)
    : message(msg), code(error_code) {
}

} // namespace youdidit
} // namespace xswl

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

// ========== ClaimerState 相关实现 ==========

std::string to_string(const ClaimerState &state) {
    if (state.is_offline()) return "Offline";
    if (state.is_paused()) return "Paused";
    if (state.is_busy()) return "Busy";
    if (state.has_claimed_tasks()) return "Working";
    if (state.is_idle()) return "Idle";
    return "Unknown";
}

tl::optional<ClaimerState> claimer_state_from_string(const std::string &str) {
    if (str == "Idle") {
        return ClaimerState{}; // defaults represent Idle
    } else if (str == "Working") {
        ClaimerState s; s.claimed_task_count = 1; s.max_concurrent = 2; return s;
    } else if (str == "Busy") {
        ClaimerState s; s.claimed_task_count = 1; s.max_concurrent = 1; return s;
    } else if (str == "Paused") {
        ClaimerState s; s.accepting_new_tasks = false; return s;
    } else if (str == "Offline") {
        ClaimerState s; s.online = false; return s;
    }
    return tl::nullopt;
}

// ========== TaskResult 实现 ==========

TaskResult::TaskResult()
    : summary(""), output() {
}

TaskResult::TaskResult(const std::string &summary)
    : summary(summary), output() {
}

TaskResult::TaskResult(const Error &error)
    : error(error) {
}

// ========== Error 实现 ==========

Error::Error(const std::string &msg, ErrorCode error_code)
    : message(msg), code(error_code) {
}

} // namespace youdidit
} // namespace xswl

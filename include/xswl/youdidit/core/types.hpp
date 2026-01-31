/**
 * @file types.hpp
 * @brief 核心类型定义
 * 
 * 包含任务系统的所有基础类型、枚举和结构体定义
 */

#ifndef XSWL_YOUDIDIT_CORE_TYPES_HPP
#define XSWL_YOUDIDIT_CORE_TYPES_HPP

#include <tl/optional.hpp>
#include <tl/expected.hpp>
#include <string>
#include <map>
#include <chrono>

namespace xswl {
namespace youdidit {

// ========== 类型别名 ==========

/**
 * @brief 任务唯一标识符类型
 */
using TaskId = std::string;

/**
 * @brief 时间戳类型
 */
using Timestamp = std::chrono::system_clock::time_point;

// ========== 优先级常量 ==========

/**
 * @brief 任务优先级范围
 */
namespace Priority {
    constexpr int MIN = 0;        ///< 最低优先级
    constexpr int LOW = 25;       ///< 低优先级
    constexpr int NORMAL = 50;    ///< 正常优先级（默认值）
    constexpr int HIGH = 75;      ///< 高优先级
    constexpr int MAX = 100;      ///< 最高优先级
    constexpr int DEFAULT = NORMAL;
} // namespace Priority

// ========== 枚举类型 ==========

/**
 * @brief 任务状态枚举
 */
enum class TaskStatus {
    Draft,       ///< 待发布 - 任务已创建，尚未发布到平台
    Published,   ///< 已发布 - 任务已发布，等待申领
    Claimed,     ///< 已申领 - 已被处理者申领，准备开始
    Processing,  ///< 处理中 - 正在执行处理
    Paused,      ///< 暂停 - 暂时停止处理
    Completed,   ///< 已完成 - 任务成功完成
    Failed,      ///< 失败 - 任务处理失败
    Cancelled,   ///< 已取消 - 发布者取消任务
    Abandoned    ///< 已放弃 - 申领者放弃任务
};

/**
 * @brief 将任务状态转换为字符串
 * @param status 任务状态
 * @return 状态的字符串表示
 */
std::string to_string(TaskStatus status);

/**
 * @brief 从字符串解析任务状态
 * @param str 状态字符串
 * @return 解析成功返回对应状态，失败返回空
 */
tl::optional<TaskStatus> task_status_from_string(const std::string &str);

/**
 * @brief 申领者状态结构（正交属性）
 *
 * 申领者真实状态由以下正交维度决定：
 *  - 在线/离线（online）
 *  - 是否接收新任务（accepting_new_tasks）
 *  - 当前活跃任务数与最大并发（active_task_count / max_concurrent）
 *
 * 以下是常用的便捷判断：Idle / Working / Busy / Paused / Offline
 */
struct ClaimerState {
    bool online{true};                      ///< 可用状态：true=在线，false=离线
    bool accepting_new_tasks{true};         ///< 是否接收新任务：true=接收，false=暂停接收
    int active_task_count{0};               ///< 当前活跃任务数
    int max_concurrent{5};                  ///< 最大并发任务数（默认值）

    // 只要在线且无活跃任务即视为闲置
    bool is_idle() const noexcept { return online && accepting_new_tasks && active_task_count == 0; }
    // 只要有活跃任务（active_task_count > 0）即视为工作中
    bool is_working() const noexcept { return online && active_task_count > 0; }
    // 只要活跃任务数达到最大并发即视为忙碌
    bool is_busy() const noexcept { return online && active_task_count >= max_concurrent; }
    // 在线但暂停接收新任务
    bool is_paused() const noexcept { return online && !accepting_new_tasks; }
    // 离线状态
    bool is_offline() const noexcept { return !online; }

    bool operator==(const ClaimerState &other) const noexcept {
        return online == other.online && accepting_new_tasks == other.accepting_new_tasks
            && active_task_count == other.active_task_count && max_concurrent == other.max_concurrent;
    }
    bool operator!=(const ClaimerState &other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief 将申领者状态转换为字符串（根据优先级判断单一表现形式）
 * @param state 申领者状态
 * @return 状态的字符串表示："Offline" | "Paused" | "Busy" | "Working" | "Idle"
 */
std::string to_string(const ClaimerState &state);

/**
 * @brief 从字符串解析申领者状态（返回一个代表性的 ClaimerState）
 * @param str 状态字符串
 * @return 解析成功返回对应状态，失败返回空
 */
tl::optional<ClaimerState> claimer_state_from_string(const std::string &str);

// ========== 结构体类型 ==========

/**
 * @brief 错误码枚举
 * 
 * 定义系统中所有可能的错误码（强类型）
 */
enum class ErrorCode {
    SUCCESS = 0,                      ///< 操作成功
    
    // 任务相关错误 (1001-1999)
    TASK_NOT_FOUND = 1001,            ///< 任务不存在
    TASK_STATUS_INVALID = 1002,       ///< 任务状态不允许此操作
    TASK_ALREADY_CLAIMED = 1003,      ///< 任务已被其他申领者申领
    TASK_CATEGORY_MISMATCH = 1004,    ///< 任务分类不匹配
    TASK_INVALID_STATE = 1005,        ///< 任务状态无效
    TASK_EXECUTION_FAILED = 1006,     ///< 任务执行失败
    TASK_NO_HANDLER = 1007,           ///< 任务没有设置处理函数
    
    // 申领者相关错误 (2001-2999)
    CLAIMER_NOT_FOUND = 2001,         ///< 申领者不存在
    CLAIMER_TOO_MANY_TASKS = 2002,    ///< 申领者已达最大并发任务数
    CLAIMER_ROLE_MISMATCH = 2003,     ///< 申领者角色不匹配
    CLAIMER_BLOCKED = 2004,           ///< 申领者被发布者禁止
    CLAIMER_NOT_ALLOWED = 2005,       ///< 申领者不在允许列表中
    
    // 平台相关错误 (3001-3999)
    PLATFORM_QUEUE_FULL = 3001,       ///< 平台任务队列已满
    PLATFORM_NO_AVAILABLE_TASK = 3002 ///< 没有可申领的任务
};

/**
 * @brief 将错误码转换为整数
 */
inline int to_int(ErrorCode code) noexcept {
    return static_cast<int>(code);
}

/**
 * @brief 错误信息结构体
 * 
 * 用于表示操作失败时的错误信息
 */
struct Error {
    std::string message;  ///< 错误消息
    ErrorCode code;       ///< 错误码（强类型）
    
    /**
     * @brief 构造函数
     * @param msg 错误消息
     * @param error_code 错误码
     */
    explicit Error(const std::string &msg, ErrorCode error_code = ErrorCode::SUCCESS);

    // 明确默认拷贝/移动语义（避免模板在不同单元实例化导致的隐式删除）
    Error(const Error&) = default;
    Error(Error&&) = default;
    Error &operator=(const Error&) = default;
    Error &operator=(Error&&) = default;
    
    /**
     * @brief 获取错误码的整数值
     */
    int code_value() const noexcept { return to_int(code); }
};

/**
 * @brief 任务执行结果
 *
 * 封装任务执行的结果信息，包括成功/失败、摘要、输出数据和结构化错误
 */
struct TaskResult {
    std::string summary;                             ///< 结果摘要
    std::string output;                              ///< 输出数据（自由文本，推荐序列化为 JSON 或类似格式）
    Error error{ "", ErrorCode::SUCCESS };           ///< 失败时的错误信息（ErrorCode::SUCCESS 表示成功）

    /**
     * @brief 默认构造函数（表示成功且摘要为空）
     */
    TaskResult();

    // 明确默认拷贝/移动语义，避免模板类型在不同编译单元导致隐式删除
    TaskResult(const TaskResult &)=default;
    TaskResult(TaskResult &&)=default;
    TaskResult &operator=(const TaskResult&)=default;
    TaskResult &operator=(TaskResult&&)=default;

    /**
     * @brief 成功结果构造函数
     * @param summary 结果摘要
     */
    explicit TaskResult(const std::string &summary);

    TaskResult(const Error &error);

    /**
     * @brief 检查是否成功
     */
    bool ok() const noexcept { return error.code == ErrorCode::SUCCESS; }
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_CORE_TYPES_HPP

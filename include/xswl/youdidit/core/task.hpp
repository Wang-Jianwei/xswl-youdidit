#ifndef XSWL_YOUDIDIT_CORE_TASK_HPP
#define XSWL_YOUDIDIT_CORE_TASK_HPP

#include <xswl/youdidit/core/types.hpp>
#include <xswl/signals.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <chrono>

namespace xswl {
namespace youdidit {

class Task {
public:
    // ========== 类型定义 ==========
    using TaskHandler = std::function<tl::expected<TaskResult, std::string>(
        Task &task,
        const std::string &input
    )>;
    
    // ========== 构造与析构 ==========
    Task();
    explicit Task(const TaskId &id);
    ~Task() noexcept;
    
    // 禁止拷贝
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    
    // 允许移动
    Task(Task &&other) noexcept;
    Task &operator=(Task &&other) noexcept;
    
    // ========== Getter 方法 ==========
    const TaskId &id() const noexcept;  // ID不可变，返回引用安全
    std::string title() const;           // 返回副本，线程安全
    std::string description() const;     // 返回副本，线程安全
    int priority() const;                // 需要加锁，不标noexcept
    TaskStatus status() const noexcept;  // atomic，线程安全
    int progress() const noexcept;       // atomic，线程安全
    std::string category() const;        // 返回副本，线程安全
    std::set<std::string> tags() const;  // 返回副本，线程安全
    const Timestamp &created_at() const noexcept;  // 不可变，返回引用安全
    Timestamp published_at() const noexcept;
    Timestamp claimed_at() const noexcept;
    Timestamp started_at() const noexcept;
    Timestamp completed_at() const noexcept;
    std::string claimer_id() const;      // 返回副本，线程安全
    std::map<std::string, std::string> metadata() const;
    
    // 白名单和黑名单 (返回副本)
    std::set<std::string> whitelist() const;
    std::set<std::string> blacklist() const;
    
    // ========== Setter 方法 (Fluent API) ==========
    Task &set_title(const std::string &title);
    Task &set_description(const std::string &description);
    Task &set_priority(int priority);
    
    /**
     * @brief 直接设置状态（已废弃）
     * @deprecated 请使用语义化方法：publish(), start(), pause(), resume(), cancel()
     * @note 内部使用和测试用途仍可使用
     */
    Task &set_status(TaskStatus status);
    
    Task &set_progress(int progress);
    Task &set_category(const std::string &category);
    Task &add_tag(const std::string &tag);
    Task &remove_tag(const std::string &tag);
    Task &set_claimer_id(const std::string &claimer_id);
    Task &set_metadata(const std::string &key, const std::string &value);
    Task &remove_metadata(const std::string &key);
    
    // 白名单和黑名单操作
    Task &add_to_whitelist(const std::string &claimer_id);
    Task &remove_from_whitelist(const std::string &claimer_id);
    Task &add_to_blacklist(const std::string &claimer_id);
    Task &remove_from_blacklist(const std::string &claimer_id);
    
    // ========== 时间戳设置（内部使用）==========
    // 这些方法仅供框架内部使用，不应由外部代码调用
    Task &set_published_at(const Timestamp &timestamp);
    Task &set_claimed_at(const Timestamp &timestamp);
    Task &set_started_at(const Timestamp &timestamp);
    Task &set_completed_at(const Timestamp &timestamp);
    
    // ========== 语义化状态转换 API ==========
    /**
     * @brief 发布任务 (Draft -> Published)
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> publish();
    
    /**
     * @brief 开始执行任务 (Claimed -> Processing)
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> start();
    
    /**
     * @brief 暂停任务 (Processing -> Paused)
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> pause();
    
    /**
     * @brief 恢复任务 (Paused -> Processing)
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> resume();
    
    /**
     * @brief 取消任务 (Published -> Cancelled)
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> cancel();
    
    /**
     * @brief 标记任务完成 (Processing -> Completed)
     * @param result 任务结果
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> complete(const TaskResult &result);
    
    /**
     * @brief 标记任务失败 (Processing -> Failed)
     * @param reason 失败原因
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> fail(const std::string &reason);
    
    /**
     * @brief 放弃任务 (Claimed/Processing/Paused -> Abandoned)
     * @param reason 放弃原因
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> abandon(const std::string &reason);
    
    /**
     * @brief 重新发布任务 (Failed/Abandoned -> Published)
     * @return 成功返回void，失败返回错误
     */
    tl::expected<void, Error> republish();
    
    // ========== 业务逻辑方法 ==========
    // 设置任务处理函数
    Task &set_handler(TaskHandler handler);

    // 执行任务
    tl::expected<TaskResult, Error> execute(const std::string &input);

    // ========== 自动清理标志 ==========
    /**
     * @brief 设置是否允许在平台清理操作中自动删除此任务
     * @param auto_cleanup true 表示允许被自动清理（默认 false）
     */
    Task &set_auto_cleanup(bool auto_cleanup);

    /**
     * @brief 检查是否允许自动清理
     */
    bool auto_cleanup() const noexcept;
    
    // 状态转换验证
    bool can_transition_to(TaskStatus new_status) const noexcept;
    
    // 申领者权限检查
    bool is_claimer_allowed(const std::string &claimer_id) const noexcept;

    // 原子尝试将任务标记为已申领（Published -> Claimed）
    tl::expected<void, Error> try_claim(const std::string &claimer_id);
    
    // ========== 信号 ==========
    xswl::signal_t<Task &, TaskStatus /* old_status */, TaskStatus /* new_status */> on_status_changed;
    xswl::signal_t<Task &, int /* progress */> on_progress_updated;
    xswl::signal_t<Task &, const std::string & /* claimer_id */> on_claimed;
    xswl::signal_t<Task &> on_started;
    xswl::signal_t<Task &, const std::string & /* claimer_id */> on_abandoned;
    xswl::signal_t<Task &, const TaskResult &> on_completed;
    xswl::signal_t<Task &, const std::string & /* error_message */> on_failed;
    xswl::signal_t<Task &> on_cancelled;

    /**
     * @brief 请求取消（协作式取消）
     *
     * 当发布者或平台希望取消正在执行的任务时，调用此方法可设置取消请求标志并触发
     * `on_cancel_requested` 信号。任务处理函数应在合适的点检查 `is_cancel_requested()` 并
     * 按需中止执行。
     */
    tl::expected<void, Error> request_cancel(const std::string &reason);

    /**
     * @brief 检查是否已请求取消
     */
    bool is_cancel_requested() const noexcept;

    /**
     * @brief 取消请求信号（参数：Task&, reason）
     */
    xswl::signal_t<Task &, const std::string &> on_cancel_requested;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
    
    // 私有辅助方法
    void _trigger_status_signal(TaskStatus old_status, TaskStatus new_status);
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_CORE_TASK_HPP

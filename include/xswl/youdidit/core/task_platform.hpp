#ifndef XSWL_YOUDIDIT_CORE_TASK_PLATFORM_HPP
#define XSWL_YOUDIDIT_CORE_TASK_PLATFORM_HPP

#include <xswl/youdidit/core/task.hpp>
#include <xswl/youdidit/core/claimer.hpp>
#include <xswl/youdidit/core/task_builder.hpp>
#include <xswl/signals.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <atomic>

namespace xswl {
namespace youdidit {

class TaskPlatform : public std::enable_shared_from_this<TaskPlatform> {
public:
    // ========== 构造与析构 ==========
    TaskPlatform();
    explicit TaskPlatform(const std::string &platform_id);
    ~TaskPlatform() noexcept;

    TaskPlatform(const TaskPlatform &) = delete;
    TaskPlatform &operator=(const TaskPlatform &) = delete;
    TaskPlatform(TaskPlatform &&) noexcept = default;
    TaskPlatform &operator=(TaskPlatform &&) noexcept = default;

    // ========== 基本信息 ==========
    const std::string &platform_id() const noexcept;
    const std::string &name() const noexcept;
    TaskPlatform &set_name(const std::string &name);

    TaskPlatform &set_max_task_queue_size(size_t size);
    size_t max_task_queue_size() const noexcept;

    // ========== 任务管理 ==========
    tl::expected<TaskId, Error> publish_task(const std::shared_ptr<Task> &task);
    tl::expected<TaskId, Error> create_and_publish_task(const std::function<void(TaskBuilder &)> &configurator);

    std::shared_ptr<Task> get_task(const TaskId &task_id) const;
    bool has_task(const TaskId &task_id) const;
    /**
     * @brief 从平台移除任务
     * @param task_id 任务ID
     * @param force 如果为 true 则强制移除（即使任务已被申领），否则拒绝移除已被申领的任务
     * @return 成功返回 true，失败返回 false
     */
    bool remove_task(const TaskId &task_id, bool force = false);

    bool cancel_task(const TaskId &task_id);

    // ========== 任务查询 ==========
    struct TaskFilter {
        tl::optional<TaskStatus> status;
        tl::optional<std::string> category;
        tl::optional<int> min_priority;
        tl::optional<int> max_priority;
        std::vector<std::string> tags;
        tl::optional<std::string> claimer_id;
    };

    std::vector<std::shared_ptr<Task>> get_tasks(const TaskFilter &filter = {}) const;
    std::vector<std::shared_ptr<Task>> get_published_tasks() const;
    std::vector<std::shared_ptr<Task>> get_tasks_by_status(TaskStatus status) const;
    std::vector<std::shared_ptr<Task>> get_tasks_by_category(const std::string &category) const;
    std::vector<std::shared_ptr<Task>> get_tasks_by_priority(int min_priority, int max_priority) const;

    tl::expected<std::shared_ptr<Task>, Error> try_get_next_task() const;

    size_t task_count() const;
    size_t task_count_by_status(TaskStatus status) const;

    // ========== 清理方法 ==========
    /**
     * @brief 清理指定状态的任务
     * @param status 要清理的任务状态
     * @param only_auto_clean 如果为 true 则仅清理那些通过 Task::set_auto_cleanup(true) 标记的任务
     */
    void clear_tasks_by_status(TaskStatus status, bool only_auto_clean = true);

    /**
     * @brief 清理已完成的任务（等价于 clear_tasks_by_status(Completed, only_auto_clean)）
     */
    void clear_completed_tasks(bool only_auto_clean = true);

    // ========== 申领者管理 ==========
    void register_claimer(const std::shared_ptr<Claimer> &claimer);
    bool unregister_claimer(const std::string &claimer_id);

    std::shared_ptr<Claimer> get_claimer(const std::string &claimer_id) const;
    bool has_claimer(const std::string &claimer_id) const;
    std::vector<std::shared_ptr<Claimer>> get_claimers() const;

    size_t claimer_count() const;

    // ========== 任务申领（供 Claimer 调用） ==========
    tl::expected<std::shared_ptr<Task>, Error> claim_task(const std::shared_ptr<Claimer> &claimer, const TaskId &task_id);
    tl::expected<std::shared_ptr<Task>, Error> claim_next_task(const std::shared_ptr<Claimer> &claimer);
    tl::expected<std::shared_ptr<Task>, Error> claim_matching_task(const std::shared_ptr<Claimer> &claimer);
    std::vector<std::shared_ptr<Task>> claim_tasks_to_capacity(const std::shared_ptr<Claimer> &claimer);

    // ========== 构建器工厂 ==========
    TaskBuilder task_builder();

    // ========== 统计 ==========
    struct PlatformStatistics {
        size_t total_tasks;
        size_t published_tasks;
        size_t claimed_tasks;
        size_t processing_tasks;
        size_t completed_tasks;
        size_t failed_tasks;
        size_t abandoned_tasks;
        size_t total_claimers;
        Timestamp start_time;
    };

    PlatformStatistics get_statistics() const;

    // ========== 信号 ==========
    xswl::signal_t<const std::shared_ptr<Task>&> sig_task_published;
    xswl::signal_t<const std::shared_ptr<Task>&> sig_task_claimed;
    xswl::signal_t<const std::shared_ptr<Task>&> sig_task_started;
    xswl::signal_t<const std::shared_ptr<Task>&, const TaskResult&> sig_task_completed;
    xswl::signal_t<const std::shared_ptr<Task>&, const Error&> sig_task_failed;
    xswl::signal_t<const std::shared_ptr<Task>&> sig_task_cancelled;
    /**
     * @brief 当对正在执行/已申领任务发出取消请求时触发（参数：task, reason）
     */
    xswl::signal_t<const std::shared_ptr<Task>&, const std::string&> sig_task_cancel_requested;

    /**
     * @brief 当任务被清理（从平台删除）时触发（参数：task）
     */
    xswl::signal_t<const std::shared_ptr<Task>&> sig_task_deleted;

    xswl::signal_t<const std::shared_ptr<Claimer>&> sig_claimer_registered;
    xswl::signal_t<const std::string&> sig_claimer_unregistered;

private:
    class Impl;
    std::unique_ptr<Impl> d;

    // 私有删除辅助方法（供内部统一调用）
    bool _delete_task_internal(const TaskId &task_id, bool force = false);
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_CORE_TASK_PLATFORM_HPP

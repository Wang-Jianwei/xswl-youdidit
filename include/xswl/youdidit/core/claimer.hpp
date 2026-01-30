#ifndef XSWL_YOUDIDIT_CORE_CLAIMER_HPP
#define XSWL_YOUDIDIT_CORE_CLAIMER_HPP

#include <xswl/youdidit/core/task.hpp>
#include <xswl/signals.hpp>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>

namespace xswl {
namespace youdidit {

// 前向声明
class TaskPlatform;

/**
 * @brief 任务申领者类
 * 
 * 代表一个可以申领和执行任务的实体（工作者）
 */
class Claimer : public std::enable_shared_from_this<Claimer> {
public:
    // ========== 构造与析构 ==========
    Claimer(const std::string &id, const std::string &name);
    ~Claimer() noexcept;
    
    // 禁止拷贝，允许移动
    Claimer(const Claimer &) = delete;
    Claimer &operator=(const Claimer &) = delete;
    Claimer(Claimer &&other) noexcept;
    Claimer &operator=(Claimer &&other) noexcept;
    
    // ========== 基本属性 Getter ==========
    const std::string &id() const noexcept;  // ID不可变，返回引用安全
    std::string name() const;                 // 返回副本，线程安全
    ClaimerStatus status() const noexcept;    // 计算属性，无锁
    int max_concurrent_tasks() const noexcept;  // atomic
    int active_task_count() const noexcept;     // atomic
    
    // 角色和分类
    std::set<std::string> roles() const;       // 返回副本
    std::set<std::string> categories() const;  // 返回副本
    
    // 已申领的任务
    std::vector<std::shared_ptr<Task>> claimed_tasks() const;
    std::vector<std::shared_ptr<Task>> active_tasks() const;
    
    // 统计信息
    int total_claimed() const noexcept;
    int total_completed() const noexcept;
    int total_failed() const noexcept;
    int total_abandoned() const noexcept;
    
    // ========== 基本属性 Setter (Fluent API) ==========
    Claimer &set_name(const std::string &name);
    
    /**
     * @brief 手动设置状态（已废弃）
     * @deprecated 状态现在根据active_task_count自动计算
     * @note 如需暂停接收任务，请使用 set_paused(true)
     */
    [[deprecated("状态根据active_task_count自动计算，使用set_paused()替代")]]
    Claimer &set_status(ClaimerStatus status);
    
    /**
     * @brief 设置是否暂停接收新任务
     * @param paused true表示暂停，false表示恢复
     * @note Paused 状态：
     *       - 临时暂停接收新任务（如休息、处理紧急事务）
     *       - 仍然在线，可继续执行已申领的任务
     *       - 调用 can_claim_more() 将返回 false
     *       - 随时可通过 set_paused(false) 恢复
     */
    Claimer &set_paused(bool paused);
    
    /**
     * @brief 设置是否离线
     * @param offline true表示离线，false表示在线
     * @note Offline 状态：
     *       - 完全不可用（如网络断开、下班、系统维护）
     *       - 不接受任何新任务
     *       - 调用 can_claim_more() 将返回 false
     *       - 优先级高于 Paused：同时设置时状态显示为 Offline
     */
    Claimer &set_offline(bool offline);
    
    Claimer &set_max_concurrent(int max_concurrent);
    
    // 角色和分类管理
    Claimer &add_role(const std::string &role);
    Claimer &remove_role(const std::string &role);
    Claimer &add_category(const std::string &category);
    Claimer &remove_category(const std::string &category);
    
    // ========== 任务申领方法 ==========
    /**
     * @brief 申领指定任务
     * @param task_id 任务ID
     * @return 成功返回任务对象，失败返回错误
     */
    tl::expected<std::shared_ptr<Task>, Error> claim_task(const TaskId &task_id);
    
    /**
     * @brief 申领指定任务（直接传入Task对象）
     * @return 成功返回void，失败返回错误
     * @note 与 claim_task(TaskId) 不同，此方法不返回 Task 对象（调用者已有）
     */
    tl::expected<void, Error> claim_task(std::shared_ptr<Task> task);
    
    /**
     * @brief 从平台申领下一个优先级最高的任务
     * @return 成功返回任务对象，失败返回错误
     */
    tl::expected<std::shared_ptr<Task>, Error> claim_next_task();
    
    /**
     * @brief 申领匹配度最高的任务
     * @return 成功返回任务对象，失败返回错误
     */
    tl::expected<std::shared_ptr<Task>, Error> claim_matching_task();
    
    /**
     * @brief 申领任务直到达到最大并发数
     * @return 申领成功的任务列表
     */
    std::vector<std::shared_ptr<Task>> claim_tasks_to_capacity();
    
    // ========== 任务执行方法 ==========
    /**
     * @brief 执行任务并自动完成记账
     * @param task 任务对象
     * @param input 任务输入数据
     * @return 返回任务执行结果（成功或失败都包含在 TaskResult 中）
     * @note 此方法会自动：
     *       1. 执行任务
     *       2. 成功时调用 complete_task
     *       3. 失败时调用 abandon_task
     *       4. 更新所有统计数据
     */
    TaskResult run_task(std::shared_ptr<Task> task, const std::string &input);
    
    /**
     * @brief 执行任务并自动完成记账
     * @param task_id 任务ID
     * @param input 任务输入数据
     */
    TaskResult run_task(const TaskId &task_id, const std::string &input);
    
    /**
     * @brief 完成任务
     */
    tl::expected<void, Error> complete_task(const TaskId &task_id, const TaskResult &result);
    
    /**
     * @brief 放弃任务
     */
    tl::expected<void, Error> abandon_task(const TaskId &task_id, const std::string &reason);
    
    /**
     * @brief 暂停任务
     */
    tl::expected<void, Error> pause_task(const TaskId &task_id);
    
    /**
     * @brief 恢复任务
     */
    tl::expected<void, Error> resume_task(const TaskId &task_id);
    
    // ========== 查询方法 ==========
    /**
     * @brief 检查是否可以申领更多任务
     * @return 当以下条件全部满足时返回 true：
     *         - 未处于 Offline 状态
     *         - 未处于 Paused 状态
     *         - active_task_count < max_concurrent_tasks
     */
    bool can_claim_more() const noexcept;
    
    /**
     * @brief 检查是否有指定ID的任务
     */
    bool has_task(const TaskId &task_id) const;
    
    /**
     * @brief 获取指定ID的任务
     */
    tl::optional<std::shared_ptr<Task>> get_task(const TaskId &task_id) const;
    
    /**
     * @brief 计算与任务的匹配度（0-100）
     */
    int calculate_match_score(const std::shared_ptr<Task> &task) const;
    
    // ========== 平台关联 ==========
    /**
     * @brief 设置关联的平台
     */
    void set_platform(TaskPlatform* platform);
    
    /**
     * @brief 获取关联的平台
     */
    TaskPlatform* platform() const noexcept;
    
    // ========== 信号 ==========
    xswl::signal_t<Claimer &, std::shared_ptr<Task>> sig_task_claimed;
    xswl::signal_t<Claimer &, std::shared_ptr<Task>> sig_task_started;
    xswl::signal_t<Claimer &, std::shared_ptr<Task>, const TaskResult &> sig_task_completed;
    xswl::signal_t<Claimer &, std::shared_ptr<Task>, const std::string & /* reason */> sig_task_failed;
    xswl::signal_t<Claimer &, std::shared_ptr<Task>, const std::string & /* reason */> sig_task_abandoned;
    xswl::signal_t<Claimer &, ClaimerStatus /* old_status */, ClaimerStatus /* new_status */> sig_status_changed;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
    
    // 私有辅助方法
    tl::expected<void, Error> _check_claim_permission(const std::shared_ptr<Task> &task) const;
    void _update_statistics(TaskStatus old_status, TaskStatus new_status);
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_CORE_CLAIMER_HPP

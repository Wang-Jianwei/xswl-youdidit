#include <xswl/youdidit/core/claimer.hpp>
#include <xswl/youdidit/core/task_platform.hpp>
#include <algorithm>
#include <mutex>
#include <atomic>

namespace xswl {
namespace youdidit {

// C++11 兼容的 make_unique 实现
namespace {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique_impl(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

// ========== 内部实现类 ==========
class Claimer::Impl {
public:
    // 基本属性
    std::string id_;
    std::string name_;
    std::atomic<bool> paused_;      // 是否暂停接收任务
    std::atomic<bool> offline_;     // 是否离线
    std::atomic<int> max_concurrent_tasks_;
    std::atomic<int> active_task_count_;
    
    // 角色和分类
    std::set<std::string> roles_;
    std::set<std::string> categories_;
    
    // 已申领的任务
    std::map<TaskId, std::shared_ptr<Task>> claimed_tasks_;
    
    // 统计信息
    std::atomic<int> total_claimed_;
    std::atomic<int> total_completed_;
    std::atomic<int> total_failed_;
    std::atomic<int> total_abandoned_;
    
    // 平台关联
    TaskPlatform* platform_;
    
    // 线程同步
    mutable std::mutex data_mutex_;
    
    explicit Impl(const std::string &id, const std::string &name)
        : id_(id),
          name_(name),
          paused_(false),
          offline_(false),
          max_concurrent_tasks_(5),
          active_task_count_(0),
          total_claimed_(0),
          total_completed_(0),
          total_failed_(0),
          total_abandoned_(0),
          platform_(nullptr) {}
    
    // 计算当前状态
    ClaimerStatus calculate_status() const noexcept {
        if (offline_.load(std::memory_order_acquire)) {
            return ClaimerStatus::Offline;
        }
        if (paused_.load(std::memory_order_acquire)) {
            return ClaimerStatus::Paused;
        }
        int active = active_task_count_.load(std::memory_order_acquire);
        int max_concurrent = max_concurrent_tasks_.load(std::memory_order_acquire);
        if (active >= max_concurrent) {
            return ClaimerStatus::Busy;
        }
        return ClaimerStatus::Idle;
    }
};

// ========== 构造与析构 ==========
Claimer::Claimer(const std::string &id, const std::string &name)
    : d(make_unique_impl<Impl>(id, name)) {}

Claimer::~Claimer() noexcept = default;

Claimer::Claimer(Claimer &&other) noexcept = default;

Claimer &Claimer::operator=(Claimer &&other) noexcept = default;

// ========== 基本属性 Getter ==========
const std::string &Claimer::id() const noexcept {
    return d->id_;
}

std::string Claimer::name() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->name_;  // 返回副本，线程安全
}

ClaimerStatus Claimer::status() const noexcept {
    return d->calculate_status();  // 状态现在是计算属性
}

int Claimer::max_concurrent_tasks() const noexcept {
    return d->max_concurrent_tasks_.load(std::memory_order_acquire);
}

int Claimer::active_task_count() const noexcept {
    return d->active_task_count_.load(std::memory_order_acquire);
}

std::set<std::string> Claimer::roles() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->roles_;
}

std::set<std::string> Claimer::categories() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->categories_;
}

std::vector<std::shared_ptr<Task>> Claimer::claimed_tasks() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    std::vector<std::shared_ptr<Task>> tasks;
    for (const auto &pair : d->claimed_tasks_) {
        tasks.push_back(pair.second);
    }
    return tasks;
}

std::vector<std::shared_ptr<Task>> Claimer::active_tasks() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    std::vector<std::shared_ptr<Task>> tasks;
    for (const auto &pair : d->claimed_tasks_) {
        TaskStatus status = pair.second->status();
        if (status == TaskStatus::Claimed || 
            status == TaskStatus::Processing || 
            status == TaskStatus::Paused) {
            tasks.push_back(pair.second);
        }
    }
    return tasks;
}

int Claimer::total_claimed() const noexcept {
    return d->total_claimed_.load(std::memory_order_acquire);
}

int Claimer::total_completed() const noexcept {
    return d->total_completed_.load(std::memory_order_acquire);
}

int Claimer::total_failed() const noexcept {
    return d->total_failed_.load(std::memory_order_acquire);
}

int Claimer::total_abandoned() const noexcept {
    return d->total_abandoned_.load(std::memory_order_acquire);
}

// ========== 基本属性 Setter ==========
Claimer &Claimer::set_name(const std::string &name) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->name_ = name;
    return *this;
}

Claimer &Claimer::set_status(ClaimerStatus new_status) {
    // 已废弃：手动设置状态
    // 兼容旧代码：根据传入的状态设置对应的标志
    ClaimerStatus old_status = status();
    
    switch (new_status) {
        case ClaimerStatus::Offline:
            d->offline_.store(true, std::memory_order_release);
            d->paused_.store(false, std::memory_order_release);
            break;
        case ClaimerStatus::Paused:
            d->paused_.store(true, std::memory_order_release);
            d->offline_.store(false, std::memory_order_release);
            break;
        case ClaimerStatus::Idle:
        case ClaimerStatus::Busy:
            // Idle 和 Busy 现在是自动计算的，只清除 paused 和 offline 标志
            d->paused_.store(false, std::memory_order_release);
            d->offline_.store(false, std::memory_order_release);
            break;
    }
    
    ClaimerStatus actual_new_status = status();
    if (old_status != actual_new_status) {
        emit on_status_changed(*this, old_status, actual_new_status);
    }
    return *this;
}

Claimer &Claimer::set_paused(bool paused) {
    ClaimerStatus old_status = status();
    d->paused_.store(paused, std::memory_order_release);
    if (paused) {
        d->offline_.store(false, std::memory_order_release);  // 互斥
    }
    ClaimerStatus new_status = status();
    if (old_status != new_status) {
        emit on_status_changed(*this, old_status, new_status);
    }
    return *this;
}

Claimer &Claimer::set_offline(bool offline) {
    ClaimerStatus old_status = status();
    d->offline_.store(offline, std::memory_order_release);
    if (offline) {
        d->paused_.store(false, std::memory_order_release);  // 互斥
    }
    ClaimerStatus new_status = status();
    if (old_status != new_status) {
        emit on_status_changed(*this, old_status, new_status);
    }
    return *this;
}

Claimer &Claimer::set_max_concurrent(int max_concurrent) {
    ClaimerStatus old_status = status();
    d->max_concurrent_tasks_.store(max_concurrent, std::memory_order_release);
    ClaimerStatus new_status = status();
    // 修改并发数可能会改变 Idle/Busy 状态
    if (old_status != new_status) {
        emit on_status_changed(*this, old_status, new_status);
    }
    return *this;
}

Claimer &Claimer::add_role(const std::string &role) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->roles_.insert(role);
    return *this;
}

Claimer &Claimer::remove_role(const std::string &role) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->roles_.erase(role);
    return *this;
}

Claimer &Claimer::add_category(const std::string &category) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->categories_.insert(category);
    return *this;
}

Claimer &Claimer::remove_category(const std::string &category) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->categories_.erase(category);
    return *this;
}

// ========== 任务申领方法 ==========
tl::expected<std::shared_ptr<Task>, Error> Claimer::claim_task(const TaskId &task_id) {
    if (!d->platform_) {
        return tl::make_unexpected(Error("Platform not available", ErrorCode::CLAIMER_NOT_FOUND));
    }
    std::shared_ptr<Claimer> self;
    try {
        self = shared_from_this();
    } catch (...) {
        return tl::make_unexpected(Error("Claimer must be managed by shared_ptr", ErrorCode::CLAIMER_NOT_FOUND));
    }

    return d->platform_->claim_task(self, task_id);
}

tl::expected<void, Error> Claimer::claim_task(std::shared_ptr<Task> task) {
    if (!task) {
        return tl::make_unexpected(Error("Task is null", ErrorCode::TASK_NOT_FOUND));
    }
    
    // 检查是否可以申领更多任务
    if (!can_claim_more()) {
        return tl::make_unexpected(Error("Max concurrent tasks reached", 
                                         ErrorCode::CLAIMER_TOO_MANY_TASKS));
    }
    
    // 检查权限
    auto perm_check = _check_claim_permission(task);
    if (!perm_check.has_value()) {
        return tl::make_unexpected(perm_check.error());
    }
    
    // 并发安全地尝试将任务标记为已申领
    auto claim_result = task->try_claim(d->id_);
    if (!claim_result.has_value()) {
        return tl::make_unexpected(claim_result.error());
    }

    {
        std::lock_guard<std::mutex> lock(d->data_mutex_);
        d->claimed_tasks_[task->id()] = task;
        d->active_task_count_.fetch_add(1, std::memory_order_acq_rel);
        d->total_claimed_.fetch_add(1, std::memory_order_acq_rel);
    }
    
    // 触发信号
    emit on_task_claimed(*this, task);
    
    return {};  // 返回 void，不是 task
}

tl::expected<std::shared_ptr<Task>, Error> Claimer::claim_next_task() {
    if (!d->platform_) {
        return tl::make_unexpected(Error("Platform not available", ErrorCode::CLAIMER_NOT_FOUND));
    }
    std::shared_ptr<Claimer> self;
    try {
        self = shared_from_this();
    } catch (...) {
        return tl::make_unexpected(Error("Claimer must be managed by shared_ptr", ErrorCode::CLAIMER_NOT_FOUND));
    }

    return d->platform_->claim_next_task(self);
}

tl::expected<std::shared_ptr<Task>, Error> Claimer::claim_matching_task() {
    if (!d->platform_) {
        return tl::make_unexpected(Error("Platform not available", ErrorCode::CLAIMER_NOT_FOUND));
    }
    std::shared_ptr<Claimer> self;
    try {
        self = shared_from_this();
    } catch (...) {
        return tl::make_unexpected(Error("Claimer must be managed by shared_ptr", ErrorCode::CLAIMER_NOT_FOUND));
    }

    return d->platform_->claim_matching_task(self);
}

std::vector<std::shared_ptr<Task>> Claimer::claim_tasks_to_capacity() {
    if (!d->platform_) {
        return std::vector<std::shared_ptr<Task>>();
    }

    std::shared_ptr<Claimer> self;
    try {
        self = shared_from_this();
    } catch (...) {
        return std::vector<std::shared_ptr<Task>>();
    }

    return d->platform_->claim_tasks_to_capacity(self);
}

// ========== 任务执行方法 ==========
tl::expected<TaskResult, Error> Claimer::run_task(const TaskId &task_id, const std::string &input) {
    auto task_opt = get_task(task_id);
    if (!task_opt.has_value()) {
        return tl::make_unexpected(Error("Task not found", ErrorCode::TASK_NOT_FOUND));
    }
    return run_task(task_opt.value(), input);
}

tl::expected<TaskResult, Error> Claimer::run_task(std::shared_ptr<Task> task, const std::string &input) {
    if (!task) {
        return tl::make_unexpected(Error("Task is null", ErrorCode::TASK_NOT_FOUND));
    }
    
    // 检查任务是否属于当前申领者
    if (task->claimer_id() != d->id_) {
        return tl::make_unexpected(Error("Task is not claimed by this claimer", 
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    // 触发开始信号
    emit on_task_started(*this, task);
    
    // 执行任务
    auto result = task->execute(input);
    
    if (result.has_value()) {
        // 成功完成 - 自动调用 complete_task 记账
        TaskResult task_result = result.value();
        complete_task(task->id(), task_result);
        return task_result;
    } else {
        // 执行失败 - 自动调用 abandon_task 记账
        Error error = result.error();
        abandon_task(task->id(), error.message);
        return tl::make_unexpected(error);
    }
}

tl::expected<void, Error> Claimer::complete_task(const TaskId &task_id, const TaskResult &result) {
    auto task_opt = get_task(task_id);
    if (!task_opt.has_value()) {
        return tl::make_unexpected(Error("Task not found", ErrorCode::TASK_NOT_FOUND));
    }
    
    auto task = task_opt.value();
    TaskStatus old_status = task->status();
    
    // 如果任务是 Claimed 状态,先转换到 Processing
    if (old_status == TaskStatus::Claimed) {
        task->set_status(TaskStatus::Processing);
    }
    
    // 设置任务为已完成
    task->set_status(TaskStatus::Completed);
    task->set_progress(100);
    task->set_completed_at(std::chrono::system_clock::now());
    
    // 触发信号
    emit on_task_completed(*this, task, result);
    
    // 从已申领任务列表中移除已完成的任务，并减少活跃任务计数
    {
        std::lock_guard<std::mutex> lock(d->data_mutex_);
        auto it = d->claimed_tasks_.find(task_id);
        if (it != d->claimed_tasks_.end()) {
            d->claimed_tasks_.erase(it);
            d->active_task_count_.fetch_sub(1, std::memory_order_acq_rel);
        }
    }
    
    // 更新统计
    _update_statistics(old_status, TaskStatus::Completed);
    
    return {};
}

tl::expected<void, Error> Claimer::abandon_task(const TaskId &task_id, const std::string &reason) {
    auto task_opt = get_task(task_id);
    if (!task_opt.has_value()) {
        return tl::make_unexpected(Error("Task not found", ErrorCode::TASK_NOT_FOUND));
    }
    
    auto task = task_opt.value();
    TaskStatus old_status = task->status();
    
    // 设置任务为已放弃
    task->set_status(TaskStatus::Abandoned);
    
    // 触发信号
    emit on_task_abandoned(*this, task, reason);
    
    // 更新统计
    _update_statistics(old_status, TaskStatus::Abandoned);
    
    // 从已申领任务列表中移除
    {
        std::lock_guard<std::mutex> lock(d->data_mutex_);
        d->claimed_tasks_.erase(task_id);
        d->active_task_count_.fetch_sub(1, std::memory_order_acq_rel);
    }
    
    return {};
}

tl::expected<void, Error> Claimer::pause_task(const TaskId &task_id) {
    auto task_opt = get_task(task_id);
    if (!task_opt.has_value()) {
        return tl::make_unexpected(Error("Task not found", ErrorCode::TASK_NOT_FOUND));
    }
    
    auto task = task_opt.value();
    
    if (task->status() != TaskStatus::Processing) {
        return tl::make_unexpected(Error("Task is not processing", 
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    task->set_status(TaskStatus::Paused);
    return {};
}

tl::expected<void, Error> Claimer::resume_task(const TaskId &task_id) {
    auto task_opt = get_task(task_id);
    if (!task_opt.has_value()) {
        return tl::make_unexpected(Error("Task not found", ErrorCode::TASK_NOT_FOUND));
    }
    
    auto task = task_opt.value();
    
    if (task->status() != TaskStatus::Paused) {
        return tl::make_unexpected(Error("Task is not paused", 
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    task->set_status(TaskStatus::Processing);
    return {};
}

// ========== 查询方法 ==========
bool Claimer::can_claim_more() const noexcept {
    // Offline: 完全不可用，不接受任何任务
    if (d->offline_.load(std::memory_order_acquire)) {
        return false;
    }
    // Paused: 暂停接收新任务（但仍可执行已申领的任务）
    if (d->paused_.load(std::memory_order_acquire)) {
        return false;
    }
    // 检查并发容量
    return active_task_count() < max_concurrent_tasks();
}

bool Claimer::has_task(const TaskId &task_id) const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->claimed_tasks_.find(task_id) != d->claimed_tasks_.end();
}

tl::optional<std::shared_ptr<Task>> Claimer::get_task(const TaskId &task_id) const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    auto it = d->claimed_tasks_.find(task_id);
    if (it != d->claimed_tasks_.end()) {
        return it->second;
    }
    return tl::nullopt;
}

int Claimer::calculate_match_score(const std::shared_ptr<Task> &task) const {
    if (!task) {
        return 0;
    }
    
    int score = 0;
    
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    
    
    // 分类匹配（50分）
    if (!task->category().empty()) {
        if (d->categories_.find(task->category()) != d->categories_.end()) {
            score += 50;
        }
    }
    
    // 标签匹配（30分）
    auto task_tags = task->tags();
    int matching_tags = 0;
    for (const auto &tag : task_tags) {
        // 这里简化处理，假设标签和分类可以匹配
        if (d->categories_.find(tag) != d->categories_.end()) {
            matching_tags++;
        }
    }
    if (!task_tags.empty()) {
        score += (matching_tags * 30) / static_cast<int>(task_tags.size());
    }
    
    // 优先级加成（20分）
    // 优先级越高，加成越多
    score += (task->priority() * 20) / 100;
    
    return std::min(100, score);
}

// ========== 平台关联 ==========
void Claimer::set_platform(TaskPlatform* platform) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->platform_ = platform;
}

TaskPlatform* Claimer::platform() const noexcept {
    return d->platform_;
}

// ========== 私有辅助方法 ==========
tl::expected<void, Error> Claimer::_check_claim_permission(const std::shared_ptr<Task> &task) const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    
    // 检查任务是否允许当前申领者
    if (!task->is_claimer_allowed(d->id_)) {
        return tl::make_unexpected(Error("Claimer is not allowed to claim this task", 
                                         ErrorCode::CLAIMER_BLOCKED));
    }
    
    // 检查分类匹配（如果任务有分类要求）
    if (!task->category().empty()) {
        if (!d->categories_.empty()) {
            if (d->categories_.find(task->category()) == d->categories_.end()) {
                return tl::make_unexpected(Error("Task category does not match claimer categories", 
                                                 ErrorCode::TASK_CATEGORY_MISMATCH));
            }
        }
    }
    
    return {};
}

void Claimer::_update_statistics(TaskStatus old_status, TaskStatus new_status) {
    // 注意：active_task_count 在 complete_task 和 abandon_task 中已经处理
    // 这里只更新完成/失败计数
    if (new_status == TaskStatus::Completed) {
        d->total_completed_.fetch_add(1, std::memory_order_acq_rel);
    } else if (new_status == TaskStatus::Failed) {
        d->total_failed_.fetch_add(1, std::memory_order_acq_rel);
    } else if (new_status == TaskStatus::Abandoned) {
        d->total_abandoned_.fetch_add(1, std::memory_order_acq_rel);
    }
}

} // namespace youdidit
} // namespace xswl

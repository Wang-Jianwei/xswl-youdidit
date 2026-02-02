#include <xswl/youdidit/core/task_platform.hpp>
#include <algorithm>
#include <sstream>
#include <mutex>

namespace xswl {
namespace youdidit {

namespace {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique_impl(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    std::string generate_platform_id() {
        static std::atomic<int> counter{0};
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        std::ostringstream oss;
        oss << "platform_" << timestamp << "_" << counter.fetch_add(1);
        return oss.str();
    }
}

// ========== 内部实现类 ==========
class TaskPlatform::Impl {
public:
    std::string platform_id_;
    std::string name_;
    size_t max_queue_size_;
    Timestamp start_time_;
    std::atomic<size_t> total_completed_;
    std::atomic<size_t> total_failed_;

    mutable std::mutex tasks_mutex_;
    std::map<TaskId, std::shared_ptr<Task>> tasks_;

    mutable std::mutex claimers_mutex_;
    std::map<std::string, std::shared_ptr<Claimer>> claimers_;

    explicit Impl(const std::string &id)
        : platform_id_(id),
          max_queue_size_(10000),
          start_time_(std::chrono::system_clock::now()),
          total_completed_(0),
          total_failed_(0) {}

    bool is_task_allowed_for_claimer(const std::shared_ptr<Task> &task,
                                     const std::shared_ptr<Claimer> &claimer) const {
        if (!task || !claimer) {
            return false;
        }

        // 黑白名单权限
        if (!task->is_claimer_allowed(claimer->id())) {
            return false;
        }

        // 分类匹配（如果任务有分类要求）
        if (!task->category().empty()) {
            auto categories = claimer->categories();
            if (!categories.empty() && categories.find(task->category()) == categories.end()) {
                return false;
            }
        }

        return true;
    }
};

// ========== 构造与析构 ==========
TaskPlatform::TaskPlatform()
    : d(make_unique_impl<Impl>(generate_platform_id())) {}

TaskPlatform::TaskPlatform(const std::string &platform_id)
    : d(make_unique_impl<Impl>(platform_id)) {}

TaskPlatform::~TaskPlatform() noexcept = default;

// ========== 基本信息 ==========
const std::string &TaskPlatform::platform_id() const noexcept {
    return d->platform_id_;
}

const std::string &TaskPlatform::name() const noexcept {
    return d->name_;
}

TaskPlatform &TaskPlatform::set_name(const std::string &name) {
    d->name_ = name;
    return *this;
}

TaskPlatform &TaskPlatform::set_max_task_queue_size(size_t size) {
    d->max_queue_size_ = size;
    return *this;
}

size_t TaskPlatform::max_task_queue_size() const noexcept {
    return d->max_queue_size_;
}

// ========== 任务管理 ==========
TaskId TaskPlatform::publish_task(const std::shared_ptr<Task> &task) {
    if (!task) {
        return "";
    }

    {
        std::lock_guard<std::mutex> lock(d->tasks_mutex_);
        if (d->max_queue_size_ > 0 && d->tasks_.size() >= d->max_queue_size_) {
            return "";
        }
        d->tasks_[task->id()] = task;
    }

    // 确保状态为 Published
    if (task->status() == TaskStatus::Draft) {
        task->set_status(TaskStatus::Published);
        task->set_published_at(std::chrono::system_clock::now());
    }

    emit sig_task_published(task);
    return task->id();
}

TaskId TaskPlatform::create_and_publish_task(const std::function<void(TaskBuilder &)> &configurator) {
    TaskBuilder builder(this);
    configurator(builder);
    auto task = builder.build_and_publish();
    if (!task) {
        return "";
    }
    return publish_task(task);
}

std::shared_ptr<Task> TaskPlatform::get_task(const TaskId &task_id) const {
    std::lock_guard<std::mutex> lock(d->tasks_mutex_);
    auto it = d->tasks_.find(task_id);
    if (it != d->tasks_.end()) {
        return it->second;
    }
    return nullptr;
}

bool TaskPlatform::has_task(const TaskId &task_id) const {
    std::lock_guard<std::mutex> lock(d->tasks_mutex_);
    return d->tasks_.find(task_id) != d->tasks_.end();
}

bool TaskPlatform::_delete_task_internal(const TaskId &task_id, bool force) {
    std::shared_ptr<Task> task;
    std::string claimer_id;
    bool had_claimer = false;
    {
        std::lock_guard<std::mutex> lock(d->tasks_mutex_);
        auto it = d->tasks_.find(task_id);
        if (it == d->tasks_.end()) return false;
        task = it->second;
        claimer_id = task->claimer_id();
        had_claimer = !claimer_id.empty();
        if (!force && had_claimer) {
            // 不允许删除仍被申领的任务
            return false;
        }
        d->tasks_.erase(it);
    }

    // 如果是强制删除且任务之前被某个 Claimer 申领，尝试通知 Claimer 进行清理
    if (force && had_claimer) {
        std::shared_ptr<Claimer> claimer;
        {
            std::lock_guard<std::mutex> lock(d->claimers_mutex_);
            auto itc = d->claimers_.find(claimer_id);
            if (itc != d->claimers_.end()) {
                claimer = itc->second;
            }
        }
        if (claimer) {
            // Best-effort: 忽略返回值（可能已经被其他流程处理）
            claimer->abandon_task(task_id, "Deleted by platform (force)");
        }
    }

    // 在释放平台锁后触发删除信号
    emit sig_task_deleted(task);
    return true;
}

bool TaskPlatform::remove_task(const TaskId &task_id, bool force) {
    return _delete_task_internal(task_id, force);
}

bool TaskPlatform::cancel_task(const TaskId &task_id) {
    auto task = get_task(task_id);
    if (!task) {
        return false;
    }

    TaskStatus current = task->status();
    if (current == TaskStatus::Published) {
        // 尚未申领，可以直接取消
        task->set_status(TaskStatus::Cancelled);
        emit sig_task_cancelled(task);
        return true;
    }

    // 已被申领或正在处理：发出协作式取消请求，通知申领者
    std::string reason = "Cancelled by publisher";
    task->request_cancel(reason);
    emit sig_task_cancel_requested(task, reason);
    return true;
}

void TaskPlatform::clear_tasks_by_status(TaskStatus status, bool only_auto_clean) {
    std::vector<std::shared_ptr<Task>> deleted;
    {
        std::lock_guard<std::mutex> lock(d->tasks_mutex_);
        for (auto it = d->tasks_.begin(); it != d->tasks_.end();) {
            const auto &task = it->second;
            if (task->status() == status) {
                if (only_auto_clean && !task->auto_cleanup()) {
                    ++it;
                    continue;
                }
                // 不删除仍被申领的任务（以避免破坏申领者状态）
                if (!task->claimer_id().empty()) {
                    ++it;
                    continue;
                }
                deleted.push_back(task);
                it = d->tasks_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // 在释放平台锁后触发删除信号
    for (const auto &t : deleted) {
        emit sig_task_deleted(t);
    }
}

void TaskPlatform::clear_completed_tasks(bool only_auto_clean) {
    clear_tasks_by_status(TaskStatus::Completed, only_auto_clean);
}

// ========== 任务查询 ==========
std::vector<std::shared_ptr<Task>> TaskPlatform::get_tasks(const TaskFilter &filter) const {
    std::vector<std::shared_ptr<Task>> result;
    std::lock_guard<std::mutex> lock(d->tasks_mutex_);
    for (const auto &pair : d->tasks_) {
        const auto &task = pair.second;
        bool match = true;

        if (filter.status.has_value() && task->status() != filter.status.value()) {
            match = false;
        }
        if (match && filter.category.has_value() && task->category() != filter.category.value()) {
            match = false;
        }
        if (match && filter.min_priority.has_value() && task->priority() < filter.min_priority.value()) {
            match = false;
        }
        if (match && filter.max_priority.has_value() && task->priority() > filter.max_priority.value()) {
            match = false;
        }
        if (match && !filter.tags.empty()) {
            for (const auto &tag : filter.tags) {
                if (task->tags().find(tag) == task->tags().end()) {
                    match = false;
                    break;
                }
            }
        }
        if (match && filter.claimer_id.has_value() && task->claimer_id() != filter.claimer_id.value()) {
            match = false;
        }

        if (match) {
            result.push_back(task);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Task>> TaskPlatform::get_published_tasks() const {
    return get_tasks_by_status(TaskStatus::Published);
}

std::vector<std::shared_ptr<Task>> TaskPlatform::get_tasks_by_status(TaskStatus status) const {
    TaskFilter filter;
    filter.status = status;
    return get_tasks(filter);
}

std::vector<std::shared_ptr<Task>> TaskPlatform::get_tasks_by_category(const std::string &category) const {
    TaskFilter filter;
    filter.category = category;
    return get_tasks(filter);
}

std::vector<std::shared_ptr<Task>> TaskPlatform::get_tasks_by_priority(int min_priority, int max_priority) const {
    TaskFilter filter;
    filter.min_priority = min_priority;
    filter.max_priority = max_priority;
    return get_tasks(filter);
}

tl::expected<std::shared_ptr<Task>, Error> TaskPlatform::try_get_next_task() const {
    std::lock_guard<std::mutex> lock(d->tasks_mutex_);
    std::shared_ptr<Task> best = nullptr;
    int best_priority = -1;
    for (const auto &pair : d->tasks_) {
        const auto &task = pair.second;
        if (task->status() != TaskStatus::Published) {
            continue;
        }
        if (task->priority() > best_priority) {
            best_priority = task->priority();
            best = task;
        }
    }

    if (!best) {
        return tl::make_unexpected(Error("No published task", ErrorCode::PLATFORM_NO_AVAILABLE_TASK));
    }
    return best;
}

size_t TaskPlatform::task_count() const {
    std::lock_guard<std::mutex> lock(d->tasks_mutex_);
    return d->tasks_.size();
}

size_t TaskPlatform::task_count_by_status(TaskStatus status) const {
    size_t count = 0;
    std::lock_guard<std::mutex> lock(d->tasks_mutex_);
    for (const auto &pair : d->tasks_) {
        if (pair.second->status() == status) {
            count++;
        }
    }
    return count;
}

// ========== 申领者管理 ==========
void TaskPlatform::register_claimer(const std::shared_ptr<Claimer> &claimer) {
    if (!claimer) {
        return;
    }
    claimer->set_platform(this);
    {
        std::lock_guard<std::mutex> lock(d->claimers_mutex_);
        d->claimers_[claimer->id()] = claimer;
    }
    emit sig_claimer_registered(claimer);
}

bool TaskPlatform::unregister_claimer(const std::string &claimer_id) {
    std::shared_ptr<Claimer> removed;
    {
        std::lock_guard<std::mutex> lock(d->claimers_mutex_);
        auto it = d->claimers_.find(claimer_id);
        if (it == d->claimers_.end()) {
            return false;
        }
        removed = it->second;
        d->claimers_.erase(it);
    }
    emit sig_claimer_unregistered(claimer_id);
    return true;
}

std::shared_ptr<Claimer> TaskPlatform::get_claimer(const std::string &claimer_id) const {
    std::lock_guard<std::mutex> lock(d->claimers_mutex_);
    auto it = d->claimers_.find(claimer_id);
    if (it != d->claimers_.end()) {
        return it->second;
    }
    return nullptr;
}

bool TaskPlatform::has_claimer(const std::string &claimer_id) const {
    std::lock_guard<std::mutex> lock(d->claimers_mutex_);
    return d->claimers_.find(claimer_id) != d->claimers_.end();
}

std::vector<std::shared_ptr<Claimer>> TaskPlatform::get_claimers() const {
    std::vector<std::shared_ptr<Claimer>> result;
    std::lock_guard<std::mutex> lock(d->claimers_mutex_);
    for (const auto &pair : d->claimers_) {
        result.push_back(pair.second);
    }
    return result;
}

size_t TaskPlatform::claimer_count() const {
    std::lock_guard<std::mutex> lock(d->claimers_mutex_);
    return d->claimers_.size();
}

// ========== 任务申领 ==========
tl::expected<std::shared_ptr<Task>, Error> TaskPlatform::claim_task(const std::shared_ptr<Claimer> &claimer, const TaskId &task_id) {
    if (!claimer) {
        return tl::make_unexpected(Error("Claimer is null", ErrorCode::CLAIMER_NOT_FOUND));
    }
    if (!claimer->can_claim_more()) {
        return tl::make_unexpected(Error("Max concurrent tasks reached", ErrorCode::CLAIMER_TOO_MANY_TASKS));
    }

    auto task = get_task(task_id);
    if (!task) {
        return tl::make_unexpected(Error("Task not found", ErrorCode::TASK_NOT_FOUND));
    }

    if (task->status() != TaskStatus::Published) {
        return tl::make_unexpected(Error("Task not in Published status", ErrorCode::TASK_ALREADY_CLAIMED));
    }

    if (!d->is_task_allowed_for_claimer(task, claimer)) {
        return tl::make_unexpected(Error("Claimer not allowed", ErrorCode::CLAIMER_BLOCKED));
    }

    // 调用 Claimer 自身的申领逻辑
    auto result = claimer->claim_task(task);
    if (result.has_value()) {
        emit sig_task_claimed(task);
        return task;  // 返回 task 对象
    }
    return tl::make_unexpected(result.error());
}

tl::expected<std::shared_ptr<Task>, Error> TaskPlatform::claim_next_task(const std::shared_ptr<Claimer> &claimer) {
    if (!claimer) {
        return tl::make_unexpected(Error("Claimer is null", ErrorCode::CLAIMER_NOT_FOUND));
    }
    if (!claimer->can_claim_more()) {
        return tl::make_unexpected(Error("Max concurrent tasks reached", ErrorCode::CLAIMER_TOO_MANY_TASKS));
    }

    std::shared_ptr<Task> best = nullptr;
    int best_priority = -1;
    {
        std::lock_guard<std::mutex> lock(d->tasks_mutex_);
        for (const auto &pair : d->tasks_) {
            const auto &task = pair.second;
            if (task->status() != TaskStatus::Published) {
                continue;
            }
            if (!d->is_task_allowed_for_claimer(task, claimer)) {
                continue;
            }
            if (task->priority() > best_priority) {
                best_priority = task->priority();
                best = task;
            }
        }
    }

    if (!best) {
        return tl::make_unexpected(Error("No available task", ErrorCode::PLATFORM_NO_AVAILABLE_TASK));
    }

    return claim_task(claimer, best->id());
}

tl::expected<std::shared_ptr<Task>, Error> TaskPlatform::claim_matching_task(const std::shared_ptr<Claimer> &claimer) {
    if (!claimer) {
        return tl::make_unexpected(Error("Claimer is null", ErrorCode::CLAIMER_NOT_FOUND));
    }
    if (!claimer->can_claim_more()) {
        return tl::make_unexpected(Error("Max concurrent tasks reached", ErrorCode::CLAIMER_TOO_MANY_TASKS));
    }

    std::shared_ptr<Task> best = nullptr;
    int best_score = -1;
    int best_priority = -1;
    {
        std::lock_guard<std::mutex> lock(d->tasks_mutex_);
        for (const auto &pair : d->tasks_) {
            const auto &task = pair.second;
            if (task->status() != TaskStatus::Published) {
                continue;
            }
            if (!d->is_task_allowed_for_claimer(task, claimer)) {
                continue;
            }
            int score = claimer->calculate_match_score(task);
            if (score > best_score || (score == best_score && task->priority() > best_priority)) {
                best_score = score;
                best_priority = task->priority();
                best = task;
            }
        }
    }

    if (!best) {
        return tl::make_unexpected(Error("No available task", ErrorCode::PLATFORM_NO_AVAILABLE_TASK));
    }

    return claim_task(claimer, best->id());
}

std::vector<std::shared_ptr<Task>> TaskPlatform::claim_tasks_to_capacity(const std::shared_ptr<Claimer> &claimer) {
    std::vector<std::shared_ptr<Task>> claimed;
    while (claimer && claimer->can_claim_more()) {
        auto result = claim_matching_task(claimer);
        if (!result.has_value()) {
            break;
        }
        claimed.push_back(result.value());
    }
    return claimed;
}

// ========== 构建器工厂 ==========
TaskBuilder TaskPlatform::task_builder() {
    return TaskBuilder(this);
}

// ========== 统计 ==========
TaskPlatform::PlatformStatistics TaskPlatform::get_statistics() const {
    PlatformStatistics stats{};
    stats.start_time = d->start_time_;

    std::lock_guard<std::mutex> lock_tasks(d->tasks_mutex_);
    stats.total_tasks = d->tasks_.size();
    stats.published_tasks = 0;
    stats.claimed_tasks = 0;
    stats.processing_tasks = 0;
    stats.completed_tasks = 0;
    stats.failed_tasks = 0;
    stats.abandoned_tasks = 0;

    for (const auto &pair : d->tasks_) {
        auto status = pair.second->status();
        switch (status) {
            case TaskStatus::Published: stats.published_tasks++; break;
            case TaskStatus::Claimed: stats.claimed_tasks++; break;
            case TaskStatus::Processing: stats.processing_tasks++; break;
            case TaskStatus::Completed: stats.completed_tasks++; break;
            case TaskStatus::Failed: stats.failed_tasks++; break;
            case TaskStatus::Abandoned: stats.abandoned_tasks++; break;
            default: break;
        }
    }

    std::lock_guard<std::mutex> lock_claimers(d->claimers_mutex_);
    stats.total_claimers = d->claimers_.size();

    return stats;
}

} // namespace youdidit
} // namespace xswl

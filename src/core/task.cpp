#include <xswl/youdidit/core/task.hpp>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <sstream>

// C++11 兼容的 make_unique 实现
namespace {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique_impl(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

namespace xswl {
namespace youdidit {

// ========== 内部实现类 ==========
class Task::Impl {
public:
    // 基本属性
    TaskId id_;
    std::string title_;
    std::string description_;
    int priority_;
    std::atomic<TaskStatus> status_;
    std::atomic<int> progress_;
    std::string category_;
    std::set<std::string> tags_;
    
    // 时间戳
    Timestamp created_at_;
    std::atomic<std::chrono::system_clock::time_point::rep> published_at_;
    std::atomic<std::chrono::system_clock::time_point::rep> claimed_at_;
    std::atomic<std::chrono::system_clock::time_point::rep> started_at_;
    std::atomic<std::chrono::system_clock::time_point::rep> completed_at_;
    
    // 申领者相关
    std::string claimer_id_;
    std::set<std::string> whitelist_;
    std::set<std::string> blacklist_;
    
    // 元数据
    std::map<std::string, std::string> metadata_;
    
    // 任务处理函数
    TaskHandler handler_;
    
    // 取消请求（协作式取消）
    std::atomic<bool> cancel_requested_{false};
    std::string cancel_reason_;

    // 自动清理标志（是否允许平台基于策略删除此任务）
    std::atomic<bool> auto_cleanup_{false};

    // 线程同步
    mutable std::mutex data_mutex_;
    mutable std::mutex handler_mutex_;
    
    explicit Impl(const TaskId &id)
        : id_(id),
          priority_(0),
          status_(TaskStatus::Draft),
          progress_(0),
          created_at_(std::chrono::system_clock::now()),
          published_at_(0),
          claimed_at_(0),
          started_at_(0),
          completed_at_(0),
          cancel_requested_(false),
          auto_cleanup_(false) {}
    
    Impl() : Impl(generate_task_id()) {}
    
    static TaskId generate_task_id() {
        static std::atomic<int> counter{0};
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        std::ostringstream oss;
        oss << "T" << timestamp << "-" << counter.fetch_add(1);
        return oss.str();
    }
    
    Timestamp to_timestamp(std::chrono::system_clock::time_point::rep rep) const {
        if (rep == 0) return Timestamp();
        return Timestamp(std::chrono::system_clock::time_point::duration(rep));
    }
    
    std::chrono::system_clock::time_point::rep from_timestamp(const Timestamp &ts) const {
        return ts.time_since_epoch().count();
    }
};

// ========== 构造与析构 ==========
Task::Task() : d(make_unique_impl<Impl>()) {}

Task::Task(const TaskId &id) : d(make_unique_impl<Impl>(id)) {}

Task::~Task() noexcept = default;

Task::Task(Task &&other) noexcept = default;

Task &Task::operator=(Task &&other) noexcept = default;

// ========== Getter 方法 ==========
const TaskId &Task::id() const noexcept {
    return d->id_;
}

std::string Task::title() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->title_;  // 返回副本，锁释放后仍安全
}

std::string Task::description() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->description_;  // 返回副本，锁释放后仍安全
}

int Task::priority() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->priority_;
}

TaskStatus Task::status() const noexcept {
    return d->status_.load(std::memory_order_acquire);
}

int Task::progress() const noexcept {
    return d->progress_.load(std::memory_order_acquire);
}

std::string Task::category() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->category_;  // 返回副本，锁释放后仍安全
}

std::set<std::string> Task::tags() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->tags_;  // 返回副本，锁释放后仍安全
}

const Timestamp &Task::created_at() const noexcept {
    return d->created_at_;
}

Timestamp Task::published_at() const noexcept {
    return d->to_timestamp(d->published_at_.load(std::memory_order_acquire));
}

Timestamp Task::claimed_at() const noexcept {
    return d->to_timestamp(d->claimed_at_.load(std::memory_order_acquire));
}

Timestamp Task::started_at() const noexcept {
    return d->to_timestamp(d->started_at_.load(std::memory_order_acquire));
}

Timestamp Task::completed_at() const noexcept {
    return d->to_timestamp(d->completed_at_.load(std::memory_order_acquire));
}

std::string Task::claimer_id() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->claimer_id_;  // 返回副本，线程安全
}

std::map<std::string, std::string> Task::metadata() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->metadata_;
}

std::set<std::string> Task::whitelist() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->whitelist_;
}

std::set<std::string> Task::blacklist() const {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    return d->blacklist_;
}

// ========== Setter 方法 ==========
Task &Task::set_title(const std::string &title) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->title_ = title;
    return *this;
}

Task &Task::set_description(const std::string &description) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->description_ = description;
    return *this;
}

Task &Task::set_priority(int priority) {
    // 约束优先级范围为 [Priority::MIN, Priority::MAX]
    int clamped_priority = std::max(Priority::MIN, std::min(Priority::MAX, priority));
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->priority_ = clamped_priority;
    return *this;
}

Task &Task::set_status(TaskStatus new_status) {
    TaskStatus old_status = d->status_.load(std::memory_order_acquire);
    
    if (old_status == new_status) {
        return *this;
    }
    
    if (!can_transition_to(new_status)) {
        return *this; // 静默失败，不改变状态
    }
    
    d->status_.store(new_status, std::memory_order_release);
    _trigger_status_signal(old_status, new_status);
    
    return *this;
}

Task &Task::set_progress(int progress) {
    int clamped_progress = std::max(0, std::min(100, progress));
    int old_progress = d->progress_.exchange(clamped_progress, std::memory_order_acq_rel);
    
    if (old_progress != clamped_progress) {
        emit sig_progress_updated(*this, clamped_progress);
    }
    
    return *this;
}

// 协作式取消请求
tl::expected<void, Error> Task::request_cancel(const std::string &reason) {
    // 任何状态均可请求取消（发布者/平台/申领者请求），只是设置标志并通知
    d->cancel_requested_.store(true, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(d->data_mutex_);
        d->cancel_reason_ = reason;
    }

    // 记录到 metadata 以便审计（ISO 8601 UTC 时间）
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif
    char timebuf[64];
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);

    set_metadata("cancel.reason", reason);
    set_metadata("cancel.requested_at", std::string(timebuf));

    emit sig_cancel_requested(*this, reason);
    return {};
}

bool Task::is_cancel_requested() const noexcept {
    return d->cancel_requested_.load(std::memory_order_acquire);
}

Task &Task::set_auto_cleanup(bool auto_cleanup) {
    d->auto_cleanup_.store(auto_cleanup, std::memory_order_release);
    return *this;
}

bool Task::auto_cleanup() const noexcept {
    return d->auto_cleanup_.load(std::memory_order_acquire);
}

Task &Task::set_category(const std::string &category) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->category_ = category;
    return *this;
}

Task &Task::add_tag(const std::string &tag) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->tags_.insert(tag);
    return *this;
}

Task &Task::remove_tag(const std::string &tag) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->tags_.erase(tag);
    return *this;
}

Task &Task::set_claimer_id(const std::string &claimer_id) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->claimer_id_ = claimer_id;
    return *this;
}

Task &Task::set_metadata(const std::string &key, const std::string &value) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->metadata_[key] = value;
    return *this;
}

Task &Task::remove_metadata(const std::string &key) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->metadata_.erase(key);
    return *this;
}

Task &Task::add_to_whitelist(const std::string &claimer_id) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->whitelist_.insert(claimer_id);
    return *this;
}

Task &Task::remove_from_whitelist(const std::string &claimer_id) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->whitelist_.erase(claimer_id);
    return *this;
}

Task &Task::add_to_blacklist(const std::string &claimer_id) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->blacklist_.insert(claimer_id);
    return *this;
}

Task &Task::remove_from_blacklist(const std::string &claimer_id) {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    d->blacklist_.erase(claimer_id);
    return *this;
}

// ========== 时间戳设置 ==========
Task &Task::set_published_at(const Timestamp &timestamp) {
    d->published_at_.store(d->from_timestamp(timestamp), std::memory_order_release);
    return *this;
}

Task &Task::set_claimed_at(const Timestamp &timestamp) {
    d->claimed_at_.store(d->from_timestamp(timestamp), std::memory_order_release);
    return *this;
}

Task &Task::set_started_at(const Timestamp &timestamp) {
    d->started_at_.store(d->from_timestamp(timestamp), std::memory_order_release);
    return *this;
}

Task &Task::set_completed_at(const Timestamp &timestamp) {
    d->completed_at_.store(d->from_timestamp(timestamp), std::memory_order_release);
    return *this;
}

// ========== 业务逻辑方法 ==========
Task &Task::set_handler(TaskHandler handler) {
    std::lock_guard<std::mutex> lock(d->handler_mutex_);
    d->handler_ = std::move(handler);
    return *this;
}

TaskResult Task::execute(const std::string &input) {
    std::lock_guard<std::mutex> lock(d->handler_mutex_);

    if (!d->handler_) {
        return Error("No handler set for task", ErrorCode::TASK_NO_HANDLER);
    }

    TaskStatus current_status = status();
    if (current_status != TaskStatus::Claimed && current_status != TaskStatus::Processing) {
        // 统一使用 TASK_STATUS_INVALID 表示“在当前状态下不允许该操作”以保持一致性
        return Error("Task must be claimed or processing to execute",
                                         ErrorCode::TASK_STATUS_INVALID);
    }

    // 设置为处理中状态
    if (current_status == TaskStatus::Claimed) {
        set_status(TaskStatus::Processing);
        set_started_at(std::chrono::system_clock::now());
        emit sig_started(*this);
    }

    // 执行任务处理函数
    TaskResult result = d->handler_(*this, input);

    if (result.ok()) {
        // 成功
        TaskResult task_result = result;
        set_status(TaskStatus::Completed);
        set_progress(100);
        set_completed_at(std::chrono::system_clock::now());
        emit sig_completed(*this, task_result);
        return task_result;
    } else {
        // 失败
        set_status(TaskStatus::Failed);
        emit sig_failed(*this, result.error);
        return result.error;
    }
}


bool Task::can_transition_to(TaskStatus new_status) const noexcept {
    TaskStatus current_status = status();
    
    if (current_status == new_status) {
        return true;
    }
    
    // 状态转换规则
    switch (current_status) {
        case TaskStatus::Draft:
            return new_status == TaskStatus::Published;
            
        case TaskStatus::Published:
            return new_status == TaskStatus::Claimed || 
                   new_status == TaskStatus::Cancelled;
            
        case TaskStatus::Claimed:
            return new_status == TaskStatus::Processing || 
                   new_status == TaskStatus::Abandoned;
            
        case TaskStatus::Processing:
            return new_status == TaskStatus::Paused || 
                   new_status == TaskStatus::Completed || 
                   new_status == TaskStatus::Failed;
            
        case TaskStatus::Paused:
            return new_status == TaskStatus::Processing || 
                   new_status == TaskStatus::Abandoned;
            
        case TaskStatus::Failed:
            return new_status == TaskStatus::Published || 
                   new_status == TaskStatus::Abandoned;
            
        case TaskStatus::Abandoned:
            return new_status == TaskStatus::Published;
            
        case TaskStatus::Completed:
        case TaskStatus::Cancelled:
            return false; // 终态，不能转换
            
        default:
            return false;
    }
}

bool Task::is_claimer_allowed(const std::string &claimer_id) const noexcept {
    std::lock_guard<std::mutex> lock(d->data_mutex_);
    
    // 1. 检查黑名单（优先级最高）
    if (d->blacklist_.find(claimer_id) != d->blacklist_.end()) {
        return false;
    }
    
    // 2. 检查白名单（如果白名单不为空）
    if (!d->whitelist_.empty()) {
        return d->whitelist_.find(claimer_id) != d->whitelist_.end();
    }
    
    // 3. 如果没有白名单限制，则允许
    return true;
}

// ========== 并发申领支持 ==========
tl::expected<void, Error> Task::try_claim(const std::string &claimer_id) {
    TaskStatus expected = TaskStatus::Published;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Claimed,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Task is not in Published state",
                                         ErrorCode::TASK_ALREADY_CLAIMED));
    }

    auto now = std::chrono::system_clock::now();
    {
        std::lock_guard<std::mutex> lock(d->data_mutex_);
        d->claimer_id_ = claimer_id;
    }
    d->claimed_at_.store(d->from_timestamp(now), std::memory_order_release);

    _trigger_status_signal(TaskStatus::Published, TaskStatus::Claimed);
    return {};
}

// ========== 语义化状态转换 API ==========
tl::expected<void, Error> Task::publish() {
    TaskStatus current = status();
    if (current != TaskStatus::Draft) {
        return tl::make_unexpected(Error("Task must be in Draft state to publish",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    TaskStatus expected = TaskStatus::Draft;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Published,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to publish task",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    set_published_at(std::chrono::system_clock::now());
    _trigger_status_signal(TaskStatus::Draft, TaskStatus::Published);
    return {};
}

tl::expected<void, Error> Task::start() {
    TaskStatus current = status();
    if (current != TaskStatus::Claimed) {
        return tl::make_unexpected(Error("Task must be in Claimed state to start",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    TaskStatus expected = TaskStatus::Claimed;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Processing,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to start task",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    set_started_at(std::chrono::system_clock::now());
    _trigger_status_signal(TaskStatus::Claimed, TaskStatus::Processing);
    emit sig_started(*this);
    return {};
}

tl::expected<void, Error> Task::pause() {
    TaskStatus current = status();
    if (current != TaskStatus::Processing) {
        return tl::make_unexpected(Error("Task must be in Processing state to pause",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    TaskStatus expected = TaskStatus::Processing;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Paused,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to pause task",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    _trigger_status_signal(TaskStatus::Processing, TaskStatus::Paused);
    return {};
}

tl::expected<void, Error> Task::resume() {
    TaskStatus current = status();
    if (current != TaskStatus::Paused) {
        return tl::make_unexpected(Error("Task must be in Paused state to resume",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    TaskStatus expected = TaskStatus::Paused;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Processing,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to resume task",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    _trigger_status_signal(TaskStatus::Paused, TaskStatus::Processing);
    return {};
}

tl::expected<void, Error> Task::cancel() {
    TaskStatus current = status();
    if (current != TaskStatus::Published) {
        return tl::make_unexpected(Error("Task must be in Published state to cancel",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    TaskStatus expected = TaskStatus::Published;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Cancelled,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to cancel task",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    _trigger_status_signal(TaskStatus::Published, TaskStatus::Cancelled);
    return {};
}

tl::expected<void, Error> Task::complete(const TaskResult &result) {
    TaskStatus current = status();
    if (current != TaskStatus::Processing) {
        return tl::make_unexpected(Error("Task must be in Processing state to complete",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    TaskStatus expected = TaskStatus::Processing;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Completed,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to complete task",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    set_progress(100);
    set_completed_at(std::chrono::system_clock::now());
    _trigger_status_signal(TaskStatus::Processing, TaskStatus::Completed);
    emit sig_completed(*this, result);
    return {};
}

tl::expected<void, Error> Task::fail(const std::string &reason) {
    TaskStatus current = status();
    if (current != TaskStatus::Processing) {
        return tl::make_unexpected(Error("Task must be in Processing state to fail",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    TaskStatus expected = TaskStatus::Processing;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Failed,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to mark task as failed",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    _trigger_status_signal(TaskStatus::Processing, TaskStatus::Failed);
    emit sig_failed(*this, Error(reason, ErrorCode::TASK_EXECUTION_FAILED));
    return {};
}

tl::expected<void, Error> Task::abandon(const std::string &reason) {
    TaskStatus current = status();
    if (current != TaskStatus::Claimed && 
        current != TaskStatus::Processing && 
        current != TaskStatus::Paused) {
        return tl::make_unexpected(Error("Task must be in Claimed, Processing or Paused state to abandon",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    // 使用 CAS 操作确保只有一个调用者能将任务转为 Abandoned
    TaskStatus expected = current;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Abandoned,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to abandon task",
                                         ErrorCode::TASK_STATUS_INVALID));
    }
    
    _trigger_status_signal(current, TaskStatus::Abandoned);
    return {};
}

tl::expected<void, Error> Task::republish() {
    TaskStatus current = status();
    if (current != TaskStatus::Failed && current != TaskStatus::Abandoned) {
        return tl::make_unexpected(Error("Task must be in Failed or Abandoned state to republish",
                                         ErrorCode::TASK_STATUS_INVALID));
    }

    // 使用 CAS 操作确保原子性
    TaskStatus expected = current;
    if (!d->status_.compare_exchange_strong(expected, TaskStatus::Published,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
        return tl::make_unexpected(Error("Failed to republish task",
                                         ErrorCode::TASK_STATUS_INVALID));
    }

    // 清除申领者信息
    {
        std::lock_guard<std::mutex> lock(d->data_mutex_);
        d->claimer_id_.clear();
    }

    set_published_at(std::chrono::system_clock::now());
    _trigger_status_signal(current, TaskStatus::Published);
    return {};
}

// ========== 私有辅助方法 ==========
void Task::_trigger_status_signal(TaskStatus old_status, TaskStatus new_status) {
    emit sig_status_changed(*this, old_status, new_status);
    
    // 触发特定状态的信号
    switch (new_status) {
        case TaskStatus::Claimed:
            emit sig_claimed(*this, claimer_id());
            break;
        case TaskStatus::Abandoned:
            emit sig_abandoned(*this, claimer_id());
            break;
        case TaskStatus::Cancelled:
            emit sig_cancelled(*this);
            break;
        default:
            break;
    }
}

} // namespace youdidit
} // namespace xswl

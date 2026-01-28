#include <xswl/youdidit/core/task_builder.hpp>
#include <algorithm>

namespace xswl {
namespace youdidit {

// 前向声明 TaskPlatform（避免循环依赖）
class TaskPlatform;

// C++11 兼容的 make_unique 实现
namespace {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique_impl(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

// ========== 内部实现类 ==========
class TaskBuilder::Impl {
public:
    TaskPlatform* platform_;
    
    // 任务属性
    std::string title_;
    std::string description_;
    int priority_;
    std::string category_;
    std::vector<std::string> tags_;
    Task::TaskHandler handler_;
    std::map<std::string, std::string> metadata_;
    std::set<std::string> whitelist_;
    std::set<std::string> blacklist_;
    
    explicit Impl(TaskPlatform* platform = nullptr)
        : platform_(platform), priority_(0) {}
    
    void reset() {
        title_.clear();
        description_.clear();
        priority_ = 0;
        category_.clear();
        tags_.clear();
        handler_ = nullptr;
        metadata_.clear();
        whitelist_.clear();
        blacklist_.clear();
    }
    
    std::vector<std::string> validate() const {
        std::vector<std::string> errors;
        
        // 规则 1: 标题不能为空
        if (title_.empty()) {
            errors.push_back("Task title cannot be empty");
        }
        
        // 规则 2: 标题长度限制（1-200字符）
        if (title_.length() > 200) {
            errors.push_back("Task title too long (max 200 characters)");
        }
        
        // 规则 3: 优先级范围（0-100）
        if (priority_ < 0 || priority_ > 100) {
            errors.push_back("Task priority must be between 0 and 100");
        }
        
        // 规则 4: 描述长度限制（最大10000字符）
        if (description_.length() > 10000) {
            errors.push_back("Task description too long (max 10000 characters)");
        }
        
        // 规则 5: 必须设置处理函数
        if (!handler_) {
            errors.push_back("Task handler must be set");
        }
        
        return errors;
    }
};

// ========== 构造与析构 ==========
TaskBuilder::TaskBuilder() : d(make_unique_impl<Impl>()) {}

TaskBuilder::TaskBuilder(TaskPlatform* platform) 
    : d(make_unique_impl<Impl>(platform)) {}

TaskBuilder::~TaskBuilder() noexcept = default;

TaskBuilder::TaskBuilder(TaskBuilder &&other) noexcept = default;

TaskBuilder &TaskBuilder::operator=(TaskBuilder &&other) noexcept = default;

// ========== Fluent API ==========
TaskBuilder &TaskBuilder::title(const std::string &title) {
    d->title_ = title;
    return *this;
}

TaskBuilder &TaskBuilder::description(const std::string &description) {
    d->description_ = description;
    return *this;
}

TaskBuilder &TaskBuilder::priority(int priority) {
    d->priority_ = priority;
    return *this;
}

TaskBuilder &TaskBuilder::category(const std::string &category) {
    d->category_ = category;
    return *this;
}

TaskBuilder &TaskBuilder::add_tag(const std::string &tag) {
    d->tags_.push_back(tag);
    return *this;
}

TaskBuilder &TaskBuilder::handler(Task::TaskHandler handler) {
    d->handler_ = std::move(handler);
    return *this;
}

TaskBuilder &TaskBuilder::metadata(const std::string &key, const std::string &value) {
    d->metadata_[key] = value;
    return *this;
}

TaskBuilder &TaskBuilder::whitelist(const std::string &claimer_id) {
    d->whitelist_.insert(claimer_id);
    return *this;
}

TaskBuilder &TaskBuilder::blacklist(const std::string &claimer_id) {
    d->blacklist_.insert(claimer_id);
    return *this;
}

// ========== 构建方法 ==========
std::shared_ptr<Task> TaskBuilder::build() {
    // 验证
    auto errors = validation_errors();
    if (!errors.empty()) {
        // 构建失败，返回空指针
        return nullptr;
    }
    
    // 创建任务
    auto task = std::make_shared<Task>();
    
    // 设置基本属性
    task->set_title(d->title_)
         .set_description(d->description_)
         .set_priority(d->priority_)
         .set_category(d->category_)
         .set_handler(d->handler_);
    
    // 设置标签
    for (const auto &tag : d->tags_) {
        task->add_tag(tag);
    }
    
    // 设置元数据
    for (const auto &pair : d->metadata_) {
        task->set_metadata(pair.first, pair.second);
    }
    
    // 设置白名单
    for (const auto &id : d->whitelist_) {
        task->add_to_whitelist(id);
    }
    
    // 设置黑名单
    for (const auto &id : d->blacklist_) {
        task->add_to_blacklist(id);
    }
    
    return task;
}

std::shared_ptr<Task> TaskBuilder::build_and_publish() {
    auto task = build();
    
    if (!task) {
        return nullptr;
    }
    
    // 如果有平台，发布任务
    if (d->platform_) {
        // 设置为 Published 状态
        task->set_status(TaskStatus::Published);
        task->set_published_at(std::chrono::system_clock::now());
        
        // 注意：实际的平台注册会在 TaskPlatform 类中实现
        // 这里只设置状态和时间戳
    } else {
        // 没有平台，只设置为 Published 状态
        task->set_status(TaskStatus::Published);
        task->set_published_at(std::chrono::system_clock::now());
    }
    
    return task;
}

// ========== 验证 ==========
bool TaskBuilder::is_valid() const {
    return validation_errors().empty();
}

std::vector<std::string> TaskBuilder::validation_errors() const {
    return d->validate();
}

// ========== 重置 ==========
TaskBuilder &TaskBuilder::reset() {
    d->reset();
    return *this;
}

} // namespace youdidit
} // namespace xswl

#ifndef XSWL_YOUDIDIT_CORE_TASK_BUILDER_HPP
#define XSWL_YOUDIDIT_CORE_TASK_BUILDER_HPP

#include <xswl/youdidit/core/task.hpp>
#include <memory>
#include <string>
#include <vector>

namespace xswl {
namespace youdidit {

// 前向声明
class TaskPlatform;

class TaskBuilder {
public:
    // ========== 构造与析构 ==========
    TaskBuilder();
    explicit TaskBuilder(TaskPlatform* platform);
    ~TaskBuilder() noexcept;
    
    // 禁止拷贝，允许移动
    TaskBuilder(const TaskBuilder &) = delete;
    TaskBuilder &operator=(const TaskBuilder &) = delete;
    TaskBuilder(TaskBuilder &&other) noexcept;
    TaskBuilder &operator=(TaskBuilder &&other) noexcept;
    
    // ========== Fluent API ==========
    TaskBuilder &title(const std::string &title);
    TaskBuilder &description(const std::string &description);
    TaskBuilder &priority(int priority);
    TaskBuilder &category(const std::string &category);
    TaskBuilder &add_tag(const std::string &tag);
    TaskBuilder &handler(Task::TaskHandler handler);
    TaskBuilder &metadata(const std::string &key, const std::string &value);
    TaskBuilder &whitelist(const std::string &claimer_id);
    TaskBuilder &blacklist(const std::string &claimer_id);

    // 设置任务是否允许被自动清理（默认 false）
    TaskBuilder &auto_cleanup(bool enable);
    
    // ========== 构建方法 ==========
    std::shared_ptr<Task> build();
    std::shared_ptr<Task> build_and_publish();
    
    // ========== 验证 ==========
    bool is_valid() const;
    std::vector<std::string> validation_errors() const;
    
    // ========== 重置 ==========
    TaskBuilder &reset();
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_CORE_TASK_BUILDER_HPP

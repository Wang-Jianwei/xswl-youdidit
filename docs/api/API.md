# xswl-youdidit API 文档

本文档详细描述了 xswl-youdidit 库的所有公开 API 接口。

## 目录

- [核心类型定义](#核心类型定义)
- [任务类 (Task)](#任务类-task)
- [任务构建器 (TaskBuilder)](#任务构建器-taskbuilder)
- [申领者类 (Claimer)](#申领者类-claimer)
- [任务平台类 (TaskPlatform)](#任务平台类-taskplatform)
- [Web 仪表板类 (WebDashboard)](#web-仪表板类-webdashboard)
- [辅助类型](#辅助类型)
- [线程安全说明](#线程安全说明)
- [错误处理](#错误处理)
- [编码规范](#编码规范)
- [任务申领模式](#任务申领模式)

---

## 核心类型定义

### TaskId

```cpp
using TaskId = std::string;
```

任务的唯一标识符类型。

### Timestamp

```cpp
using Timestamp = std::chrono::system_clock::time_point;
```

时间戳类型，用于记录各种时间信息。

### TaskStatus

```cpp
enum class TaskStatus {
    Draft,       // 待发布
    Published,   // 已发布
    Claimed,     // 已申领
    Processing,  // 处理中
    Paused,      // 暂停
    Completed,   // 已完成
    Failed,      // 失败
    Cancelled,   // 已取消
    Abandoned    // 已放弃
};
```

任务状态枚举。

**成员方法：**
```cpp
std::string to_string() const;           // 转换为字符串
static TaskStatus from_string(const std::string &str);  // 从字符串解析
```

### ClaimerState

```cpp
struct ClaimerState {
    bool online;                // 在线/离线
    bool accepting_new_tasks;   // 是否接收新任务（paused）
    int claimed_task_count;      // 当前已申领任务数
    int max_concurrent;         // 最大并发任务数
    bool is_idle() const;
    bool is_working() const;
    bool is_busy() const;
    bool is_paused() const;
    bool is_offline() const;
};
```

申领者状态描述结构（正交属性）。

**工具函数：**
```cpp
std::string to_string(const ClaimerState &state);          // 转换为字符串用于展示
static tl::optional<ClaimerState> claimer_state_from_string(const std::string &str);  // 从字符串解析（返回代表性状态）
```

### TaskResult

```cpp
struct TaskResult {
    bool success;                                    // 是否成功
    std::string summary;                             // 结果摘要
    std::string output;  // 输出数据（自由文本，建议序列化为 JSON 等）
    tl::optional<std::string> error_message;         // 错误信息（可选）
    
    // 构造函数
    TaskResult();
    TaskResult(bool success, const std::string &summary);
};
```

任务执行结果结构体。

### Error

```cpp
struct Error {
    std::string message;     // 错误消息
    int code;                // 错误码（可选）
    
    // 构造函数
    explicit Error(const std::string &msg, int error_code = 0);
};
```

错误信息结构体。

---

## 任务类 (Task)

### 类定义

```cpp
class Task {
public:
    // ========== 类型定义 ==========
    
    using TaskHandler = std::function<TaskResult(
        Task &task,
        const std::string &input
    )>;

    **注意**：`TaskHandler` 现在直接返回 `TaskResult`；若处理失败，请返回 `TaskResult(Error(...))`，例如：

```cpp
return TaskResult(Error("处理失败原因", ErrorCode::TASK_EXECUTION_FAILED));
```

    **注意**：`TaskHandler` 现在直接返回 `TaskResult`；若处理失败，请返回 `TaskResult(Error(...))`，例如：

```cpp
return tl::make_unexpected(Error("处理失败原因", ErrorCode::TASK_EXECUTION_FAILED));
```

    // ========== 构造与析构 ==========
    
    Task();
    explicit Task(const TaskId &id);
    Task(const Task &other) = delete;             // 禁止拷贝
    Task &operator=(const Task &other) = delete;   // 禁止赋值
    Task(Task&& other) noexcept;                   // 允许移动
    Task &operator=(Task&& other) noexcept;        // 允许移动赋值
    ~Task();
    
    // ========== 基本属性访问 ==========
    
    TaskId id() const noexcept;
    std::string title() const noexcept;
    std::string description() const noexcept;
    int priority() const noexcept;
    std::string category() const noexcept;
    std::vector<std::string> tags() const;
    
    // ========== 角色相关 ==========
    
    std::string publisher_id() const noexcept;
    tl::optional<std::string> assignee_id() const noexcept;
    tl::optional<std::string> claimer_id() const noexcept;
    std::string required_role() const noexcept;
    
    // ========== 申领者限制 ==========
    
    std::set<std::string> allowed_claimers() const;   // 允许的申领者列表（空=任何人都可）
    std::set<std::string> blocked_claimers() const;   // 禁止的申领者列表
    
    Task &add_allowed_claimer(const std::string &claimer_id);      // 添加到允许列表
    Task &set_allowed_claimers(const std::set<std::string> &ids);  // 设置整个允许列表
    Task &clear_allowed_claimers();                                // 清空允许列表（允许所有人）
    
    Task &add_blocked_claimer(const std::string &claimer_id);      // 添加到禁止列表
    Task &set_blocked_claimers(const std::set<std::string> &ids);  // 设置整个禁止列表
    Task &clear_blocked_claimers();                                // 清空禁止列表
    
    // 检查申领者是否被允许申领此任务
    bool is_claimer_allowed(const std::string &claimer_id) const;
    
    // ========== 状态和进度 ==========
    
    TaskStatus status() const noexcept;            // 线程安全，原子操作
    int progress() const noexcept;                 // 线程安全，原子操作
    
    // ========== 时间信息 ==========
    
    Timestamp created_at() const;
    Timestamp published_at() const;
    tl::optional<Timestamp> claimed_at() const;

    tl::optional<Timestamp> started_at() const;
    tl::optional<Timestamp> completed_at() const;
    Timestamp deadline() const;
    
    // ========== 奖励信息 ==========
    
    int reward_points() const;
    std::string reward_type() const;
    
    // ========== 结果和数据 ==========
    
    tl::optional<TaskResult> result() const;
    std::map<std::string, std::string> metadata() const;  // 线程安全，返回副本
    std::string get_metadata(const std::string &key) const;  // 线程安全
    
    // ========== Fluent API 设置方法 ==========
    
    Task &set_title(const std::string &title);
    Task &set_description(const std::string &desc);
    Task &set_priority(int priority);                      // 线程安全
    Task &set_category(const std::string &category);
    Task &add_tag(const std::string &tag);
    Task &set_tags(const std::vector<std::string> &tags);
    
    Task &set_publisher_id(const std::string &id);
    Task &set_assignee_id(const std::string &id);
    Task &set_required_role(const std::string &role);
    
    Task &set_status(TaskStatus status);                   // 线程安全
    Task &set_progress(int progress);                      // 线程安全
    
    Task &set_deadline(const Timestamp &deadline);
    Task &set_reward_points(int points);
    Task &set_reward_type(const std::string &type);
    
    Task &set_result(const TaskResult &result);            // 线程安全
    Task &set_metadata(const std::string &key, const std::string &value);  // 线程安全
    Task &set_handler(TaskHandler handler);
    
    // ========== 任务执行 ==========
    
    TaskResult execute(
        const std::string &input = ""
    );
    
    // ========== 状态转换验证 ==========
    
    bool can_transition_to(TaskStatus new_status) const;
    std::vector<TaskStatus> valid_next_states() const;

    // ========== 信号槽接口 ==========
    
    xswl::signal_t<TaskStatus> &sig_status_changed();
    xswl::signal_t<int> &sig_progress_updated();
    xswl::signal_t<const TaskId &> &sig_published();
    xswl::signal_t<const TaskId &, const std::string &> &sig_claimed();
    xswl::signal_t<const TaskId &> &sig_started();
    xswl::signal_t<const TaskId &, const TaskResult &> &sig_completed();
    xswl::signal_t<const TaskId &, const std::string &> &sig_failed();
    xswl::signal_t<int, int> &sig_priority_changed();  // (old_priority, new_priority)

    // ========== 工具方法 ==========
    
    std::string to_string() const;                     // 转换为字符串表示
    nlohmann::json to_json() const;                    // 转换为 JSON（可选功能）
    
private:
    // 内部实现细节
    class Impl;
    std::unique_ptr<Impl> d;  // Pimpl 指针，使用简洁的 'd' 命名
};
```

### 接口说明：取消（cancel）行为说明

- `cancel()`（Published -> Cancelled）：仅当任务处于 `Published` 状态时成功执行，
    会将状态原子性地设置为 `Cancelled` 并触发 `sig_cancelled` 信号。
- 如果任务处于 `Claimed`、`Processing` 或其他非 `Published` 状态，
    调用 `cancel()` 将返回错误（`ErrorCode::TASK_STATUS_INVALID`），**不会**强制中断处理函数。
- 对于已被申领或正在处理的任务，应使用 `request_cancel(reason)`（平台会在 `cancel_task` 中发起），
    以实现协作式取消（`is_cancel_requested()` / `sig_cancel_requested`）。

**示例**：

```cpp
// 取消尚未被申领的任务
auto res = task.cancel(); // 只有在 Published 时会成功
if (!res) {
    // 失败（可能已被申领或不允许取消）
}
```

### 接口说明：任务自动清理（auto_cleanup）与平台清理 API

- `Task::set_auto_cleanup(bool)`：设置任务是否允许被平台的清理操作自动删除（默认 `false`）。
- `TaskPlatform::clear_tasks_by_status(TaskStatus status, bool only_auto_clean = true)`：清理处于指定状态的任务。默认只清理 `auto_cleanup == true` 的任务；如果 `only_auto_clean == false`，则不再检查该标志。
- `TaskPlatform::clear_completed_tasks(bool only_auto_clean = true)`：便捷方法，用于清理 `Completed` 状态的任务。

重要说明：平台在清理任务时会**跳过仍被申领**（`claimer_id` 非空）的任务以避免破坏申领者的状态；清理动作会触发 `sig_task_deleted` 信号供外部监听。

**示例**：

```cpp
// 仅清理允许自动清理的已完成任务
platform.clear_completed_tasks(true);

// 强制清理所有已完成任务（不考虑 auto_cleanup）
platform.clear_completed_tasks(false);
```

### 接口说明：取消请求与审计元数据

- `request_cancel(reason)`：向任务发出协作式取消请求（会设置 `is_cancel_requested()` 标志并触发 `on_cancel_requested` 信号）。
- 审计信息：取消原因与时间将被记录到任务 `metadata` 中，使用键名：
    - `cancel.reason`（取消原因字符串）
    - `cancel.requested_at`（ISO 8601 UTC 时间戳，例如 `2026-01-28T12:34:56Z`）

**示例**：

```cpp
// 发布者请求取消正在执行的任务
platform->cancel_task(task_id);

// 申领者/处理函数可通过以下方式检查取消请求：
if (task.is_cancel_requested()) {
    // 中止并清理
}
```

### 使用示例

```cpp
// 创建任务
Task task("task-001");
task.set_title("数据处理任务")
    .set_priority(5)
    .set_category("data_processing")
    .add_tag("urgent")
    .set_handler([](Task &t, const auto &input) {
        t.set_progress(50);
        // ... 业务逻辑
        return TaskResult("完成");
    });

// 监听状态变化
task.sig_status_changed().connect([](TaskStatus status) {
    std::cout << "状态变化: " << status.to_string() << std::endl;
});

// 执行任务
auto result = task.execute("/data/input.csv");
```

### 接口说明：Offline 与 Paused 的区别

- **Offline（离线）**：完全不可用，例如网络断开、下班或系统维护。
    - 不接受任何新任务（`can_claim_more()` 返回 `false`）。
    - 已申领但未完成的任务可能需要由平台重新调度或标记为需要重试。
    - 优先级高于 `Paused`（若同时设置，状态应显示为 `Offline`）。

- **Paused（暂停）**：临时暂停接收新任务，例如短暂休息或处理紧急事务。
    - 暂停期间不再申领新任务（`can_claim_more()` 返回 `false`）。
    - 可以继续执行已经申领的任务。
    - 可随时通过 `set_paused(false)` 恢复接收任务。

**状态优先级（高→低）**： `Offline > Paused > Busy > Idle`

**代码行为要点**：

- `can_claim_more()` 在 `offline == true` 或 `paused == true` 时都应返回 `false`。
- `status()` 的计算顺序应为：先检查 `offline`，再检查 `paused`，随后检查并发容量（`Busy/Idle`）。

**示例**：

```cpp
// 完全离线（不接任务）
claimer.set_offline(true);

// 午休或临时中断（不接新任务，但继续处理已申领的任务）
claimer.set_paused(true);

// 恢复接收任务
claimer.set_paused(false);
claimer.set_offline(false);
```
---

## 任务构建器 (TaskBuilder)

### 类定义

```cpp
class TaskBuilder {
public:
    // ========== 构造函数 ==========
    
    TaskBuilder();
    explicit TaskBuilder(TaskPlatform* platform);
    
    // ========== Fluent API 配置方法 ==========
    
    TaskBuilder &title(const std::string &title);
    TaskBuilder &description(const std::string &desc);
    TaskBuilder &priority(int priority);
    TaskBuilder &category(const std::string &category);
    TaskBuilder &tag(const std::string &tag);
    TaskBuilder &tags(const std::vector<std::string> &tags);
    
    TaskBuilder &publisher_id(const std::string &id);
    TaskBuilder &assignee_id(const std::string &id);
    TaskBuilder &required_role(const std::string &role);
    TaskBuilder &assign_to_role(const std::string &role);  // 别名
    
    TaskBuilder &allowed_claimer(const std::string &claimer_id);           // 添加允许的申领者
    TaskBuilder &allowed_claimers(const std::set<std::string> &ids);       // 设置整个允许列表
    TaskBuilder &blocked_claimer(const std::string &claimer_id);           // 添加禁止的申领者
    TaskBuilder &blocked_claimers(const std::set<std::string> &ids);       // 设置整个禁止列表
    
    TaskBuilder &deadline(const Timestamp &deadline);
    TaskBuilder &deadline_in(const std::chrono::seconds& duration);  // 相对时间
    
    TaskBuilder &reward_points(int points);
    TaskBuilder &reward_type(const std::string &type);
    
    TaskBuilder &metadata(const std::string &key, const std::string &value);
    TaskBuilder &metadata(const std::map<std::string, std::string> &data);
    
    TaskBuilder &handler(Task::TaskHandler handler);
    
    // ========== 构建方法 ==========
    
    std::shared_ptr<Task> build();                         // 构建任务对象
    std::shared_ptr<Task> build_and_publish();             // 构建并发布到平台
    
    // ========== 工具方法 ==========
    
    TaskBuilder &reset();                                  // 重置构建器
    bool is_valid() const;                                 // 检查配置是否有效
    std::vector<std::string> validation_errors() const;    // 获取验证错误
    
private:
    class Impl;
    std::unique_ptr<Impl> d;  // Pimpl 指针
};
```

### 验证规则

`is_valid()` 方法会检查以下条件：

| 验证项 | 条件 | 错误信息 |
|--------|------|----------|
| **标题** | 非空字符串 | "Title is required" |
| **优先级** | 范围 1-10 | "Priority must be between 1 and 10" |
| **截止时间** | 必须晚于当前时间 | "Deadline must be in the future" |
| **处理函数** | handler 必须设置 | "Handler is required" |
| **奖励积分** | 非负数 | "Reward points must be non-negative" |

```cpp
// 验证示例
auto builder = platform->task_builder()
    .title("")
    .priority(15);

if (!builder.is_valid()) {
    for (const auto &error : builder.validation_errors()) {
        std::cerr << "Validation error: " << error << std::endl;
    }
}
```

### 使用示例

```cpp
auto platform = std::make_shared<TaskPlatform>();

auto task = platform->task_builder()
    .title("数据分析")
    .priority(8)
    .category("analytics")
    .deadline_in(std::chrono::hours(24))
    .handler([](Task &t, const auto &input) {
        // 业务逻辑
        return TaskResult("完成");
    })
    .build_and_publish();
```

---

## 申领者类 (Claimer)

### 类定义

```cpp
class Claimer : public std::enable_shared_from_this<Claimer> {
public:
    // ========== 构造与析构 ==========
    
    Claimer(const std::string &id, const std::string &name);
    Claimer(const Claimer &) = delete;
    Claimer &operator=(const Claimer &) = delete;
    ~Claimer() noexcept;
    
    // ========== 基本属性访问 ==========
    
    std::string id() const noexcept;
    std::string name() const noexcept;
    std::string role() const noexcept;
    std::vector<std::string> skills() const;
    std::set<std::string> categories() const;
    int max_concurrent_tasks() const noexcept;
    
    // ========== 状态信息 ==========
    
    ClaimerState status() const noexcept;                 // 线程安全，返回描述性状态结构
    int claimed_task_count() const noexcept;                // 线程安全，原子操作
    std::vector<TaskId> claimed_tasks() const;             // 线程安全，返回副本
    
    // ========== 统计信息 ==========
    
    int total_completed() const noexcept;
    int total_failed() const noexcept;
    int total_abandoned() const noexcept;
    double success_rate() const noexcept;
    int reputation_points() const noexcept;
    int total_rewards() const noexcept;
    
    Timestamp registered_at() const noexcept;
    Timestamp last_active_at() const noexcept;
    
    // ========== Fluent API 设置方法 ==========
    
    Claimer &set_name(const std::string &name);
    Claimer &set_role(const std::string &role);
    Claimer &add_skill(const std::string &skill);
    Claimer &set_skills(const std::vector<std::string> &skills);
    Claimer &add_category(const std::string &category);
    Claimer &set_categories(const std::set<std::string> &categories);
    Claimer &set_max_concurrent(int max_count);            // 线程安全
    // set_status 已移除：使用 set_paused()/set_offline()/set_max_concurrent() 等方法
    
    // ========== 申领策略配置 ==========
    
    // 是否强制检查任务 category 与申领者 categories 的匹配
    // true: 严格模式（默认）- 任务 category 必须在申领者支持的 categories 中
    // false: 宽松模式 - 允许申领任意 category
    Claimer &set_strict_category_matching(bool strict);
    bool is_strict_category_matching() const;
    
    // ========== 任务申领 ==========
    
    // 方式 1：申领指定的任务
    // 失败情况：任务不存在、状态不是 Published、已达最大并发数、角色不匹配等
    tl::expected<std::shared_ptr<Task>, Error> claim_task(const TaskId &task_id);  // 线程安全
    
    // 方式 2：从平台队列中自动申领下一个任务（FIFO）
    // 自动从 Published 状态的任务队列中选择优先级最高的任务
    tl::expected<std::shared_ptr<Task>, Error> claim_next_task();  // 线程安全
    
    // 方式 3：基于申领者技能/分类自动匹配申领
    // 优先选择与申领者技能/分类最匹配的任务
    tl::expected<std::shared_ptr<Task>, Error> claim_matching_task();  // 线程安全
    
    // 方式 4：一次性申领任务至达到最大并发数
    // 返回成功申领的任务列表
    std::vector<std::shared_ptr<Task>> claim_tasks_to_capacity();  // 线程安全
    
    // ========== 任务处理 ==========
    
    tl::expected<void, Error> execute_task(
        const TaskId &task_id,
        const std::string &input = ""
    );
    
    // ========== 手动任务控制 ==========
    
    tl::expected<void, Error> start_task(const TaskId &task_id);
    tl::expected<void, Error> pause_task(const TaskId &task_id);
    tl::expected<void, Error> resume_task(const TaskId &task_id);
    tl::expected<void, Error> update_progress(const TaskId &task_id, int progress);
    tl::expected<void, Error> complete_task(const TaskId &task_id, const TaskResult &result);  // 线程安全
    tl::expected<void, Error> fail_task(const TaskId &task_id, const std::string &reason);
    tl::expected<void, Error> abandon_task(const TaskId &task_id, const std::string &reason);
    
    // ========== 查询方法 ==========
    
    std::vector<std::shared_ptr<Task>> get_active_tasks() const;  // 线程安全
    tl::optional<std::shared_ptr<Task>> get_task(const TaskId &task_id) const;
    bool has_task(const TaskId &task_id) const;
    bool can_claim_more() const;                           // 线程安全
    
    // ========== 统计查询 ==========
    
    struct Statistics {
        int total_completed;
        int total_failed;
        int total_abandoned;
        double success_rate;
        int reputation_points;
        int total_rewards;
        Timestamp first_task_at;
        Timestamp last_task_at;
        std::chrono::seconds average_task_duration;
    };
    
    Statistics get_statistics() const;
    
    // ========== 信号槽接口 ==========
    
    xswl::signal_t<const std::shared_ptr<Task>&> &sig_task_assigned();
    xswl::signal_t<const std::shared_ptr<Task>&> &sig_task_started();
    xswl::signal_t<const TaskId&, int> &sig_progress_updated();
    xswl::signal_t<const std::shared_ptr<Task>&, const TaskResult&> &sig_task_completed();
    xswl::signal_t<const std::shared_ptr<Task>&, const Error&> &sig_task_failed();
    xswl::signal_t<const std::shared_ptr<Task>&, const std::string&> &sig_task_abandoned();
    xswl::signal_t<ClaimerState> &sig_status_changed();
    
    // ========== 工具方法 ==========
    
    std::string to_string() const;
    nlohmann::json to_json() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;  // Pimpl 指针
};
```

### 使用示例

```cpp
// 创建申领者
auto claimer = std::make_shared<Claimer>("worker-001", "Alice");
claimer->set_role("DataProcessor")
       ->add_skill("data_analysis")
       ->add_skill("machine_learning")
       ->add_category("analytics")
       ->add_category("data_processing")
       ->set_max_concurrent(5);
       // 注：Category 严格匹配默认启用，申领者只能申领指定 category 的任务

// 监听任务分配
claimer->sig_task_assigned().connect([claimer](const auto &task) {
    std::cout << "收到任务: " << task->title() << std::endl;
    
    claimer->execute_task(task->id());  // 或传入输入数据字符串
});

// 申领方式 1：申领指定的任务
auto result = claimer->claim_task("task-001");
if (result) {
    std::cout << "申领成功" << std::endl;
}

// 申领方式 2：从队列自动申领下一个优先级最高的任务（推荐用于简单场景）
auto task1 = claimer->claim_next_task();
if (task1) {
    std::cout << "自动申领任务: " << task1.value()->title() << std::endl;
}

// 申领方式 3：基于技能自动匹配任务（推荐用于专业化场景）
// 会自动查找与该申领者技能和分类最匹配的任务
auto task2 = claimer->claim_matching_task();
if (task2) {
    std::cout << "匹配申领任务: " << task2.value()->title() << std::endl;
}

// 申领方式 4：一次性申领至最大并发数（推荐用于高效场景）
std::vector<std::shared_ptr<Task>> tasks = claimer->claim_tasks_to_capacity();
std::cout << "申领了 " << tasks.size() << " 个任务" << std::endl;
```

---

## 任务平台类 (TaskPlatform)

### 类定义

```cpp
class TaskPlatform : public std::enable_shared_from_this<TaskPlatform> {
public:
    // ========== 构造与析构 ==========
    
    TaskPlatform();
    explicit TaskPlatform(const std::string &platform_id);
    TaskPlatform(const TaskPlatform &) = delete;
    TaskPlatform &operator=(const TaskPlatform &) = delete;
    ~TaskPlatform();
    
    // ========== 基本信息 ==========
    
    std::string platform_id() const;
    std::string name() const;
    TaskPlatform &set_name(const std::string &name);
    
    // ========== 配置方法 ==========
    
    TaskPlatform &set_log_file(const std::string &path);
    TaskPlatform &set_max_task_queue_size(size_t size);
    
    // ========== 任务管理 ==========
    
    TaskId publish_task(const std::shared_ptr<Task> &task);  // 线程安全
    TaskId create_and_publish_task(const std::function<void(TaskBuilder &)> &configurator);
    
    std::shared_ptr<Task> get_task(const TaskId &task_id) const;  // 线程安全
    bool has_task(const TaskId &task_id) const;
    bool remove_task(const TaskId &task_id);
    bool cancel_task(const TaskId &task_id);
    
    // ========== 任务查询 ==========
    
    struct TaskFilter {
        tl::optional<TaskStatus> status;
        tl::optional<std::string> category;
        tl::optional<int> min_priority;
        tl::optional<int> max_priority;
        std::vector<std::string> tags;
        tl::optional<std::string> publisher_id;
        tl::optional<std::string> claimer_id;
    };
    
    std::vector<std::shared_ptr<Task>> get_tasks(const TaskFilter& filter = {}) const;
    std::vector<std::shared_ptr<Task>> get_published_tasks() const;
    std::vector<std::shared_ptr<Task>> get_tasks_by_status(TaskStatus status) const;
    std::vector<std::shared_ptr<Task>> get_tasks_by_category(const std::string &category) const;
    std::vector<std::shared_ptr<Task>> get_tasks_by_priority(int min, int max) const;
    
    tl::expected<std::shared_ptr<Task>, Error> try_get_next_task();  // 线程安全
    tl::expected<std::shared_ptr<Task>, Error> try_get_next_task_for_role(const std::string &role);
    
    size_t task_count() const;
    size_t task_count_by_status(TaskStatus status) const;
    
    // ========== 申领者管理 ==========
    
    void register_claimer(const std::shared_ptr<Claimer> &claimer);  // 线程安全
    bool unregister_claimer(const std::string &claimer_id);
    
    std::shared_ptr<Claimer> get_claimer(const std::string &claimer_id) const;
    bool has_claimer(const std::string &claimer_id) const;
    
    std::vector<std::shared_ptr<Claimer>> get_claimers() const;
    std::vector<std::shared_ptr<Claimer>> get_claimers_by_state(const ClaimerState &state) const; // 通过 ClaimerState 过滤申领者列表
    std::vector<std::shared_ptr<Claimer>> get_claimers_by_role(const std::string &role) const;
    
    size_t claimer_count() const;
    
    // ========== 任务分派 ==========
    
    tl::expected<void, Error> assign_task(const TaskId &task_id, const std::string &claimer_id);
    tl::expected<void, Error> auto_assign_task(const TaskId &task_id);  // 自动分配
    
    // ========== 构建器工厂 ==========
    
    TaskBuilder task_builder();
    
    // ========== 平台统计 ==========
    
    struct PlatformStatistics {
        // 任务统计
        size_t total_tasks;
        size_t published_tasks;
        size_t claimed_tasks;
        size_t processing_tasks;
        size_t completed_tasks;
        size_t failed_tasks;
        size_t abandoned_tasks;                // 放弃的任务数
        
        // 申领者统计
        size_t total_claimers;
        size_t idle_claimers;
        size_t busy_claimers;
        size_t offline_claimers;               // 离线的申领者数
        
        // 性能指标
        double avg_task_duration_seconds;      // 平均任务处理时间（秒）
        double tasks_per_hour;                 // 平台吞吐量（任务/小时）
        double task_completion_rate;           // 任务完成率 (0.0-1.0)
        double claimer_success_rate;           // 申领者成功率 (0.0-1.0)
        
        // 时间信息
        Timestamp platform_start_time;         // 平台启动时间
        std::chrono::seconds uptime;           // 运行时长
        Timestamp last_update;                 // 最后更新时间
    };
    
    PlatformStatistics get_statistics() const;
    
    // ========== 信号槽接口 ==========
    
    xswl::signal_t<const std::shared_ptr<Task>&> &sig_task_published();
    xswl::signal_t<const std::shared_ptr<Task>&> &sig_task_claimed();
    xswl::signal_t<const std::shared_ptr<Task>&> &sig_task_started();
    xswl::signal_t<const std::shared_ptr<Task>&, const TaskResult&> &sig_task_completed();
    xswl::signal_t<const std::shared_ptr<Task>&, const std::string&> &sig_task_failed();
    xswl::signal_t<const std::shared_ptr<Task>&> &sig_task_cancelled();
    
    xswl::signal_t<const std::shared_ptr<Claimer>&> &sig_claimer_registered();
    xswl::signal_t<const std::string&> &sig_claimer_unregistered();
    
    // ========== 工具方法 ==========
    
    void clear_completed_tasks();
    void clear_all_tasks();
    
    std::string to_string() const;
    nlohmann::json to_json() const;
    
    // ========== 日志和监控 ==========
    
    void set_log_level(const std::string &level);  // "debug", "info", "warning", "error"
    void enable_performance_monitoring(bool enable);
    
private:
    class Impl;
    std::unique_ptr<Impl> d;  // Pimpl 指针
};
```

### 使用示例

```cpp
// 创建平台
auto platform = std::make_shared<TaskPlatform>("platform-001");
platform->set_name("主平台")
        ->set_log_file("platform.log");

// 发布任务
auto task = platform->task_builder()
    .title("数据处理")
    .priority(5)
    .handler([](Task &t, const auto &input) {
        return TaskResult("完成");
    })
    .build_and_publish();

// 注册申领者
auto claimer = std::make_shared<Claimer>("worker-001", "Alice");
platform->register_claimer(claimer);

// 查询任务
TaskPlatform::TaskFilter filter;
filter.status = TaskStatus::Published;
filter.min_priority = 5;
auto tasks = platform->get_tasks(filter);

// 查看统计
auto stats = platform->get_statistics();
std::cout << "总任务数: " << stats.total_tasks << std::endl;
std::cout << "完成率: " << stats.task_completion_rate * 100 << "%" << std::endl;
std::cout << "运行时长: " << stats.uptime.count() << " 秒" << std::endl;
```

---

## Web 仪表板类 (WebDashboard)

> **设计说明**：`WebDashboard` 是独立于 `TaskPlatform` 的可选组件，遵循单一职责原则。
> - `TaskPlatform` 专注于任务调度和管理
> - `WebDashboard` 专注于监控、可视化和数据分析
> - 不需要 Web 功能的用户无需引入 HTTP 相关依赖

**完整 API 文档请参阅 [Web API 文档](docs/web/WEB_API.md)**，包含：
- WebDashboard、MetricsExporter、EventLog、TimeReplay 等类的完整定义
- HTTP REST API 端点详细说明
- WebSocket 实时推送 API
- 数据结构和错误码定义

### 快速示例

```cpp
// 创建平台和仪表板
auto platform = std::make_shared<TaskPlatform>("platform-001");
auto dashboard = std::make_shared<WebDashboard>(platform.get());

// 配置并启动
dashboard->set_update_interval(1000)
         ->set_max_event_history(10000)
         ->start_server(8080);

// 获取实时指标（复用 PlatformStatistics）
auto metrics = dashboard->get_metrics();
std::cout << "总任务数: " << metrics.total_tasks << std::endl;
std::cout << "完成率: " << metrics.task_completion_rate * 100 << "%" << std::endl;

// 时间回放
auto replay = dashboard->get_time_replay();
auto snapshot = replay->get_snapshot_at(Timestamp::now() - std::chrono::minutes(30));
```

---

## 辅助类型

### TaskReference

```cpp
class TaskReference {
public:
    TaskReference(const std::shared_ptr<Task> &task);
    
    // 便捷操作
    void start();
    void pause();
    void resume();
    void update_progress(int progress);
    void complete(const TaskResult &result);
    void complete(const std::string &summary);
    void fail(const std::string &reason);
    void abandon(const std::string &reason);
    
    // 访问底层任务
    std::shared_ptr<Task> task() const;
    TaskId id() const;
    
private:
    std::shared_ptr<Task> task_;
};
```

---

## 线程安全说明

以下 API 保证线程安全：

### Task 类
- `status()` / `set_status()`
- `progress()` / `set_progress()`
- `metadata()` / `get_metadata()` / `set_metadata()`
- `set_result()`
- `set_priority()`

### Claimer 类
- `status()` / `set_status()`
- `claimed_task_count()`
- `claimed_tasks()`
- `claim_task()` / `claim_next_task()` / `claim_matching_task()` / `claim_tasks_to_capacity()`
- `complete_task()`
- `get_active_tasks()`
- `set_max_concurrent()`
- `can_claim_more()`

### TaskPlatform 类
- `publish_task()`
- `get_task()`
- `register_claimer()`
- `try_get_next_task()`

---

## 错误处理

所有可能失败的操作都使用 `tl::expected<T, Error>` 返回类型，支持优雅的错误处理：

```cpp
auto result = claimer->claim_task(task_id);
if (result) {
    // 成功
    auto task = result.value();
    // ...
} else {
    // 失败
    auto error = result.error();
    std::cerr << "错误: " << error.message << std::endl;
}
```

---

## 版本信息

- **API 版本**: 1.0.0
- **最后更新**: 2026-01-27
- **兼容性**: C++11 及以上

### 错误码定义

| 错误码 | 名称 | 描述 |
|--------|------|------|
| 0 | `SUCCESS` | 操作成功（默认） |
| 1001 | `TASK_NOT_FOUND` | 任务不存在 |
| 1002 | `TASK_STATUS_INVALID` | 任务状态不允许此操作 |
| 1003 | `TASK_ALREADY_CLAIMED` | 任务已被其他申领者申领 |
| 1004 | `TASK_CATEGORY_MISMATCH` | 任务分类不匹配 |
| 2001 | `CLAIMER_NOT_FOUND` | 申领者不存在 |
| 2002 | `CLAIMER_TOO_MANY_TASKS` | 申领者已达最大并发任务数 |
| 2003 | `CLAIMER_ROLE_MISMATCH` | 申领者角色不匹配 |
| 2004 | `CLAIMER_BLOCKED` | 申领者被发布者禁止 |
| 2005 | `CLAIMER_NOT_ALLOWED` | 申领者不在允许列表中 |
| 3001 | `PLATFORM_QUEUE_FULL` | 平台任务队列已满 |
| 3002 | `PLATFORM_NO_AVAILABLE_TASK` | 没有可申领的任务 |

---

## 编程风格规范

本库遵循以下编程风格规范，确保代码的一致性和可维护性：

### 私有成员函数命名

所有类的私有成员函数采用下划线前缀命名，以区分公有和私有接口：

```cpp
class Task {
public:
    // 公有接口 - 不带下划线，Getter 使用简洁命名
    Task &set_status(TaskStatus status);
    TaskStatus status() const noexcept;
    
private:
    // 私有实现 - 带下划线前缀
    void _validate_status_transition(TaskStatus new_status);
    void _trigger_status_changed_signal();
    void _update_internal_state();
};
```

### 引用类型写法

所有引用参数和返回值采用右对齐的写法，引用符号靠近变量名/标识符：

```cpp
class Task {
public:
    // 参数引用 - 右对齐（& 靠近参数名）
    Task &set_title(const std::string &title);
    Task &set_priority(int priority);
    
    // 返回引用 - 用于支持链式调用
    Task &set_status(TaskStatus status);
    
    // 常引用参数
    void process_data(const std::string &input);
    
    // 返回常引用 - Getter 方法使用简洁命名
    const std::string &title() const noexcept;
    const std::map<std::string, std::string> &metadata() const;
};

// 变量声明
void example_function() {
    Task &task_ref = some_task;
    const std::string &title = task.title();
}
```

### 异常安全修饰符（noexcept）

根据函数的异常情况，合理使用 `noexcept` 修饰符。不同的函数应该标注异常安全等级：

```cpp
class Claimer {
public:
    // 基本操作 - 不抛出异常，使用简洁命名
    ClaimerState status() const noexcept;
    int claimed_task_count() const noexcept;
    std::string id() const noexcept;
    
    // 可能失败的操作 - 使用 expected 返回，不标注 noexcept
    tl::expected<std::shared_ptr<Task>, Error> claim_task(const TaskId &task_id);
    tl::expected<void, Error> execute_task(const TaskId &task_id, 
                     const std::string &input = "");
    
    // 移动操作 - 声明 noexcept
    Claimer(Claimer&& other) noexcept;
    Claimer &operator=(Claimer&& other) noexcept;
    
    // 析构函数 - 通常 noexcept
    ~Claimer() noexcept;
    
private:
    // 只读访问 - noexcept
    bool _is_valid_state() const noexcept;
    
    // 内部修改 - 不标注 noexcept
    void _update_internal_state();
};
```

**何时使用 noexcept**：

| 函数类型 | 是否 noexcept | 说明 |
|---------|-------------|------|
| **Getter/查询方法** | ✓ 是 | 仅读取数据，不修改状态 |
| **简单计算** | ✓ 是 | 仅进行简单运算，不访问外部资源 |
| **移动操作** | ✓ 是 | 移动构造/赋值应该不抛出异常 |
| **析构函数** | ✓ 是 | 析构中不应抛出异常 |
| **交互操作** | ✗ 否 | 任务申领、执行等可能失败，使用 `expected` 返回 |
| **IO 操作** | ✗ 否 | 文件读写、网络操作可能失败 |
| **内部修改** | ✗ 否 | 涉及复杂状态变化的操作 |

### Pimpl 模式中的实现类指针命名

采用 Pimpl（Pointer to Implementation）模式时，内部实现类指针使用简洁的 `d` 作为变量名：

```cpp
class Task {
public:
    Task();
    ~Task() noexcept;
    
    // 公有接口 - Getter 使用简洁命名
    Task &set_status(TaskStatus status);
    TaskStatus status() const noexcept;
    
private:
    class Impl;  // 前向声明
    std::unique_ptr<Impl> d;  // 使用 'd' 而不是 'pimpl_' 或 'impl_'
};

// 实现文件 (task.cpp)
class Task::Impl {
public:
    TaskStatus status;
    std::string title;
    int progress;
    std::map<std::string, std::string> metadata;
    
    // 内部方法
    void validate_transition(TaskStatus new_status);
    void trigger_signals();
};

Task::Task() : d(std::make_unique<Impl>()) {
    d->status = TaskStatus::Draft;
    d->progress = 0;
}

Task &Task::set_status(TaskStatus status) {
    if (d->status == status) return *this;
    
    // 内部操作通过 d 访问
    d->validate_transition(status);
    d->status = status;
    d->trigger_signals();
    return *this;
}

TaskStatus Task::status() const noexcept {
    return d->status;  // 直接通过 d 访问内部实现
}

Task::~Task() = default;  // std::unique_ptr 会自动清理 d
```

**Pimpl 模式的优势**：

```
┌─────────────────────────────────────┐
│         Task (public header)         │
├─────────────────────────────────────┤
│  - set_status()                     │
│  - status()                         │
│  - std::unique_ptr<Impl> d          │
└─────────────────────────────────────┘
                  ↓
         (二进制边界)
                  ↓
┌─────────────────────────────────────┐
│    Task::Impl (implementation)       │
├─────────────────────────────────────┤
│  - status                           │
│  - title                            │
│  - progress                         │
│  - metadata                         │
│  - _validate_transition()           │
│  - _trigger_signals()               │
└─────────────────────────────────────┘

优点：
  ✓ ABI 稳定性 - 修改实现不需要重新编译客户端代码
  ✓ 快速编译 - 头文件中不需要完整的类定义
  ✓ 隐藏实现细节 - 完整的信息隐藏
  ✓ 灵活设计 - 可以随时改变内部实现
```

**变量命名示例**：

```cpp
// ✓ 正确用法
class Claimer {
private:
    class Impl;
    std::unique_ptr<Impl> d;  // 简洁且易于识别
    
    void _update_task_list();  // 私有方法带下划线
    bool _is_task_valid(const TaskId &id) const noexcept;
};

// ✗ 避免这样做
class Claimer {
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;   // 过于冗长
    std::unique_ptr<Impl> impl_;    // 容易与 Impl 混淆
    std::unique_ptr<Impl> m_impl;   // 混合命名风格
};
```

### 综合示例

以下示例展示了所有风格规范的综合应用：

```cpp
class Claimer : public std::enable_shared_from_this<Claimer> {
public:
    // 构造与析构
    Claimer(const std::string &id, const std::string &name);
    ~Claimer() noexcept;
    
    // 禁止拷贝
    Claimer(const Claimer &) = delete;
    Claimer &operator=(const Claimer &) = delete;
    
    // 允许移动
    Claimer(Claimer &&other) noexcept;
    Claimer &operator=(Claimer &&other) noexcept;
    
    // ========== Getter 方法 (noexcept, 简洁命名) ==========
    const std::string &id() const noexcept;
    const std::string &name() const noexcept;
    ClaimerState status() const noexcept;
    int claimed_task_count() const noexcept;
    
    // ========== Setter 方法 (返回引用用于链式调用) ==========
    // set_status 已移除：使用 set_paused()/set_offline()/set_max_concurrent() 等方法
    Claimer &set_role(const std::string &role);
    Claimer &set_max_concurrent(int max_count);
    Claimer &add_skill(const std::string &skill);
    Claimer &add_category(const std::string &category);
    
    // ========== 业务操作方法 ==========
    tl::expected<std::shared_ptr<Task>, Error> claim_task(const TaskId &task_id);
    tl::expected<std::shared_ptr<Task>, Error> claim_next_task();
    std::vector<std::shared_ptr<Task>> get_active_tasks() const;
    
    // ========== 信号槽接口 ==========
    xswl::signal_t<const std::shared_ptr<Task> &> &sig_task_assigned();
    xswl::signal_t<const std::shared_ptr<Task> &> &sig_task_started();
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
    
    // 私有方法 - 以下划线开头
    bool _validate_state() noexcept;
    void _update_status();
    tl::expected<void, Error> _check_claim_permission(const TaskId &task_id) const;
    void _trigger_status_changed_signal() noexcept;
};

// 使用示例
auto claimer = std::make_shared<Claimer>("worker-001", "Alice");
claimer->set_role("DataProcessor")
       .add_skill("data_analysis")
       .add_category("analytics")
       .set_max_concurrent(5);

const std::string &name = claimer->name();     // 正确：简洁命名，右对齐引用
ClaimerState status = claimer->status();      // 正确：noexcept 方法

auto result = claimer->claim_task("task-001");  // 返回 expected，不抛出异常
if (result) {
    std::cout << "Claim successful" << std::endl;
}
```

---

## 注意事项

1. **智能指针管理**: 所有主要对象（Task, Claimer）都应使用 `std::shared_ptr` 管理
2. **移动语义**: Task 和 Claimer 禁止拷贝，但支持移动
3. **信号槽连接**: 使用 `enable_shared_from_this` 确保对象生命周期正确
4. **线程安全**: 标记为线程安全的 API 可在多线程环境下直接使用
5. **异常安全**: API 保证基本异常安全性，使用 RAII 管理资源

---

## 任务申领方式详解

### 四种申领方式对比

| 申领方式 | 方法 | 使用场景 | 优点 | 缺点 |
|---------|------|---------|------|------|
| **指定申领** | `claim_task(task_id)` | 知道具体任务 ID 时 | 直接、精确 | 需要事先知道任务 ID |
| **队列申领** | `claim_next_task()` | 按优先级顺序处理 | 自动、简单、公平 | 无法考虑申领者能力 |
| **匹配申领** | `claim_matching_task()` | 充分利用申领者专长 | 高效、精准匹配 | 计算开销较大 |
| **批量申领** | `claim_tasks_to_capacity()` | 高吞吐场景 | 一次申领多个、高效 | 占用更多申领者资源 |

### 申领流程详解

#### 1. 指定申领流程

```
申领者调用 claim_task(task_id)
    ↓
检查申领者状态 (是否超过最大并发数)
    ↓ (失败) 返回 Error: Too many active tasks
    ↓ (成功)
检查任务状态 (必须是 Published)
    ↓ (失败) 返回 Error: Task status is not Published
    ↓ (成功)
检查申领者权限:
  1. 检查申领者是否在禁止列表中
     ↓ (是) 返回 Error: Claimer is blocked by publisher
  2. 如果有允许列表，检查申领者是否在允许列表中
     ↓ (不在) 返回 Error: Claimer is not in allowed list
    ↓ (成功)
验证申领者角色 (如果任务有 required_role)
    ↓ (失败) 返回 Error: Role mismatch
    ↓ (成功)
如果启用严格 category 匹配模式:
  检查任务 category 是否在申领者 categories 中
    ↓ (失败) 返回 Error: Category mismatch
    ↓ (成功)
更新任务状态: Published → Claimed
    ↓
设置任务的 claimer_id
    ↓
触发信号:
  - Task::sig_claimed(task_id, claimer_id)
  - Claimer::sig_task_assigned(task)
    ↓
返回成功，返回 Task 对象
```

#### 2. 队列申领流程

```
申领者调用 claim_next_task()
    ↓
查询平台的已发布任务队列
    ↓
按优先级排序，取优先级最高的任务
    ↓ (如果启用严格 category 匹配)
按 category 过滤，只考虑在申领者支持列表中的任务
    ↓
执行与指定申领相同的检查和更新流程
    ↓
返回成功的任务或 Error
```

#### 3. 匹配申领流程

```
申领者调用 claim_matching_task()
    ↓
获取申领者的 skills 和 categories
    ↓
查询所有已发布的任务
    ↓
过滤任务：
  1. 必须是 Published 状态
  2. 如果启用严格 category 匹配，category 必须在申领者支持列表中
    ↓
计算每个任务的匹配度得分:
  - 技能匹配度 (任务 tags 与申领者 skills 的交集)
  - 分类匹配度 (任务 category 是否在申领者 categories 中)
  - 优先级权重
    ↓
选择匹配度最高的任务
    ↓
执行与指定申领相同的检查和更新流程
    ↓
返回成功的任务或 Error
```

#### 4. 批量申领流程

```
申领者调用 claim_tasks_to_capacity()
    ↓
计算还能申领的任务数: 
  capacity = max_concurrent_tasks - claimed_task_count
    ↓
如果 capacity <= 0，返回空列表
    ↓
循环 capacity 次调用 claim_next_task():
  - 成功则加入结果列表
  - 失败则停止循环
    ↓
返回成功申领的任务列表
```

### 申领失败的常见原因

| 错误 | 原因 | 触发条件 | 解决方案 |
|------|------|---------|---------|
| `Too many active tasks` | 已达最大并发数 | `claimed_task_count >= max_concurrent_tasks` | 等待任务完成或增加 `max_concurrent` |
| `Task not found` | 任务 ID 不存在 | 指定的 task_id 在平台中不存在 | 确认任务 ID 正确 |
| `Task status is not Published` | 任务已被申领或已完成 | 任务状态不是 Published | 重新查询可用任务 |
| `Claimer is blocked` | 被发布者禁止 | 申领者 ID 在任务禁止列表中 | 联系发布者解除禁用 |
| `Claimer is not in allowed list` | 不在允许列表 | 任务有允许列表 + 申领者不在其中 | 获得发布者的授权 |
| `Role mismatch` | 申领者角色不符 | `required_role` 不为空且与申领者角色不匹配 | 切换到符合要求的申领者 |
| `Category mismatch` | 任务分类不匹配 | 启用严格匹配模式 + 任务 category 不在申领者 categories 中 | 扩展申领者的 categories 或禁用严格匹配 |
| `No available tasks` | 没有符合条件的任务 | 平台中无 Published 状态的任务或无匹配任务 | 等待新任务发布或扩展技能范围 |

### 最佳实践

#### 场景 1：通用工作者（接受任何任务）

```cpp
// 使用队列申领方式 - 最简单高效
while (claimer->can_claim_more()) {
    auto result = claimer->claim_next_task();
    if (!result) break;  // 没有更多任务
    
    auto task = result.value();
    claimer->execute_task(task->id());  // 使用空字符串或输入数据
}
```

#### 场景 2a：发布者限制特定申领者 - 白名单模式

```cpp
// 发布者：只允许特定申领者处理敏感任务
auto task = platform->task_builder()
    .title("处理敏感数据")
    .priority(10)
    .category("security")
    .allowed_claimer("trusted-worker-001")    // 只允许这个申领者
    .allowed_claimer("trusted-worker-002")    // 或这个
    .handler([](Task &t, const auto &input) {
        // 处理敏感数据
        return TaskResult("完成");
    })
    .build_and_publish();

// 未授权的申领者尝试申领
auto result = untrusted_worker->claim_task(task->id());
// ✗ 失败：Claimer is not in allowed list
```

#### 场景 2b：发布者禁止特定申领者 - 黑名单模式

```cpp
// 发布者：禁止特定的不可靠申领者处理任务
auto task = platform->task_builder()
    .title("标准数据处理")
    .priority(5)
    .category("analytics")
    .blocked_claimer("unreliable-worker-001")   // 禁止这个申领者
    .blocked_claimer("suspended-worker-002")    // 禁止那个申领者
    .handler([](Task &t, const auto &input) {
        // 标准处理流程
        return TaskResult{true, "完成"};
    })
    .build_and_publish();

// 被禁止的申领者尝试申领
auto result = unreliable_worker->claim_task(task->id());
// ✗ 失败：Claimer is blocked by publisher

// 其他申领者可以申领
auto result2 = other_worker->claim_task(task->id());
// ✓ 成功
```

```cpp
// 使用匹配申领方式 + 严格 category 检查 - 精准匹配
auto claimer = std::make_shared<Claimer>("expert-001", "Expert");
claimer->set_role("DataScientist")
       ->add_skill("machine_learning")
       ->add_skill("data_analysis")
       ->add_category("analytics")
       ->add_category("ml_research")
       ->set_strict_category_matching(true);  // 启用严格匹配

while (claimer->can_claim_more()) {
    auto result = claimer->claim_matching_task();
    if (!result) {
        if (result.error().message.find("Category mismatch") != std::string::npos) {
            std::cout << "没有适合的 category 的任务" << std::endl;
        }
        break;
    }
}
```

#### 场景 3：高吞吐处理

```cpp
// 使用批量申领方式 - 一次性申领到满
auto claimer = std::make_shared<Claimer>("worker-001", "Worker");
claimer->set_max_concurrent(10);

auto tasks = claimer->claim_tasks_to_capacity();
std::cout << "一次申领了 " << tasks.size() << " 个任务" << std::endl;

for (auto &task : tasks) {
    // 异步处理每个任务
    std::thread([task, claimer]() {
        claimer->execute_task(task->id());  // 使用空字符串或输入数据
    }).detach();
}
```

#### 场景 4：指定任务处理（精确控制）

```cpp
// 使用指定申领方式 - 完全控制
auto result = claimer->claim_task("priority-task-001");
if (result) {
    auto task = result.value();
    // 立即处理高优先级任务
    claimer->execute_task(task->id(), critical_input);
} else {
    // 处理申领失败
    std::cerr << "无法申领任务: " << result.error().message << std::endl;
}
```

---

## 申领者限制机制详解

### 两种限制模式

| 模式 | 配置方法 | 行为 | 适用场景 |
|------|---------|------|---------|
| **白名单模式** | `allowed_claimers` (非空) | 只有列表中的申领者可申领 | 敏感/关键任务，权限控制严格 |
| **黑名单模式** | `blocked_claimers` (非空) | 列表中的申领者不可申领 | 普通任务，排除不可靠申领者 |
| **无限制模式** | 两个列表都为空（默认） | 所有申领者都可申领 | 普通任务，开放申领 |

### 限制优先级

当两个列表都非空时，限制的优先级如下：

```
检查流程：
1. 首先检查黑名单 (blocked_claimers)
   ↓ 申领者在黑名单中 → 申领失败（优先级最高）
   ↓ 申领者不在黑名单中 → 继续检查

2. 然后检查白名单 (allowed_claimers)
   ↓ 申领者在白名单中 → 允许申领
   ↓ 申领者不在白名单中 → 申领失败

示例：
白名单 = {worker-001, worker-002}
黑名单 = {worker-001}

worker-001 申领尝试：✗ 失败（黑名单优先）
worker-002 申领尝试：✓ 成功（在白名单且不在黑名单）
worker-003 申领尝试：✗ 失败（不在白名单）
```

### 常见使用场景

#### 场景 A：关键项目 - 只允许特定团队

```cpp
// 发布者：区块链项目任务，只允许安全认证团队处理
auto task = platform->task_builder()
    .title("区块链核心合约开发")
    .priority(10)
    .category("blockchain")
    .allowed_claimers({
        "certified-blockchain-001",
        "certified-blockchain-002",
        "team-lead-blockchain"
    })
    .handler([](Task &t, const auto &input) {
        return TaskResult("合约已通过审核");
    })
    .build_and_publish();
```

#### 场景 B：普通任务 - 排除问题申领者

```cpp
// 发布者：数据处理任务，排除质量问题的申领者
auto task = platform->task_builder()
    .title("用户数据清洗")
    .priority(5)
    .category("data_cleaning")
    .blocked_claimers({
        "worker-with-low-quality",
        "suspended-worker",
        "on-vacation-worker"
    })
    .handler([](Task &t, const auto &input) {
        return TaskResult("数据清洗完成");
    })
    .build_and_publish();
```

### 验证顺序（优先级从高到低）

```
1. 申领者状态检查 (是否超过最大并发数)
   ↓ Too many active tasks

2. 任务状态检查 (必须是 Published)
   ↓ Task status is not Published

3. 申领者权限检查 ← 新增（黑名单优先）
   ↓ Claimer is blocked / not in allowed list

4. Role 检查
   ↓ Role mismatch

5. Category 检查 (严格模式)
   ↓ Category mismatch
```

---

## Category 匹配机制详解

### 模式对比

| 模式 | 设置值 | 行为 | 适用场景 |
|------|-------|------|---------|
| **严格模式** | `true` (默认) | 只能申领 categories 中的任务 | 专业化工作者、流程可控 |
| **宽松模式** | `false` | 允许申领任意 category 的任务 | 通用工作者、多能工 |

### Category 匹配规则

```
申领者的 categories = {"analytics", "ml_research"}
任务的 category = "analytics"

严格模式 (strict=true):
  "analytics" ∈ {"analytics", "ml_research"} 
  ✓ 匹配，允许申领

严格模式 (strict=true):
  "data_processing" ∈ {"analytics", "ml_research"} 
  ✗ 不匹配，申领失败

宽松模式 (strict=false):
  无论任务 category 是什么，都允许申领
  ✓ 匹配
```

### 何时使用宽松模式

**推荐启用宽松模式的场景：**
```cpp
// 通用工作者 - 可以处理任何 category 的任务
auto general_worker = std::make_shared<Claimer>("worker-001", "General Worker");
general_worker->set_strict_category_matching(false);  // 显式设置为宽松模式

// 全栈开发 - 能处理各种任务
auto fullstack = std::make_shared<Claimer>("fullstack-001", "Full Stack");
fullstack->set_strict_category_matching(false);  // 打破类别限制
```

**推荐保持严格模式的场景（默认）：**
```cpp
// 数据科学专家 - 只想处理数据科学相关的任务
auto data_scientist = std::make_shared<Claimer>("ds-001", "Data Scientist");
data_scientist->add_category("machine_learning")
              ->add_category("analytics")
              ->add_category("research");
              // 不设置，使用默认严格模式

// 后端开发 - 只想处理后端任务
auto backend_dev = std::make_shared<Claimer>("backend-001", "Backend Dev");
backend_dev->add_category("backend_development")
           ->add_category("database_design");
           // 默认严格模式，只能申领这些 categories 的任务
```

### 与 required_role 的区别

| 属性 | required_role | category |
|------|---------------|----------|
| **来源** | 任务定义 | 任务定义 |
| **申领者端** | 申领者的 role | 申领者的 categories |
| **检查时机** | 总是检查 | 仅在严格模式检查 |
| **不匹配时** | 申领失败 | 严格模式: 申领失败 / 宽松模式: 允许 |
| **语义** | "我需要什么角色的人来做" | "这是什么类型的任务" |

### 完整示例：多模式申领

```cpp
// 创建两个申领者
auto specialist = std::make_shared<Claimer>("specialist-001", "Specialist");
specialist->add_category("backend")
          ->set_strict_category_matching(true);  // 严格模式

auto generalist = std::make_shared<Claimer>("generalist-001", "Generalist");
generalist->set_strict_category_matching(false);  // 宽松模式

// 发布任务
auto task = platform->task_builder()
    .title("前端开发任务")
    .category("frontend")
    .build_and_publish();

// 申领尝试
auto result1 = specialist->claim_task(task->id());
// ✗ 失败：Category mismatch ("frontend" ∉ {"backend"})

auto result2 = generalist->claim_task(task->id());
// ✓ 成功：宽松模式允许任意 category
```

## Web 服务器 API (WebServer)

### 类: WebServer

Web 服务器类用于为任务平台提供 HTTP 接口和实时仪表板。

#### 构造函数

```cpp
WebServer(WebDashboard *dashboard, int port = 8080);
```

**参数:**
- `dashboard`: 指向 WebDashboard 实例的指针
- `port`: HTTP 服务器监听端口（默认 8080）

#### 方法

##### start()

```cpp
void start();
```

启动 HTTP 服务器。服务器在后台线程中运行。

##### stop()

```cpp
void stop();
```

停止 HTTP 服务器并等待后台线程完成。

##### is_running()

```cpp
bool is_running() const noexcept;
```

检查服务器是否正在运行。

##### set_port(int port) / get_port()

```cpp
void set_port(int port);
int get_port() const noexcept;
```

设置/获取服务器监听端口。

##### set_host() / host()

```cpp
void set_host(const std::string &host);
const std::string &host() const noexcept;
```

设置/获取服务器监听主机地址（默认 "127.0.0.1"）。

### REST API 端点

#### GET / - 仪表板

返回实时 HTML 仪表板。

#### GET /api/metrics - 平台指标

返回 JSON 格式的平台指标。

#### GET /api/tasks - 任务列表

返回 JSON 格式的任务摘要列表。

#### GET /api/claimers - 申领者列表

返回 JSON 格式的申领者摘要列表。

#### GET /api/logs - 事件日志

返回 JSON 格式的最近事件日志。

### 使用示例

```cpp
#include <xswl/youdidit/youdidit.hpp>
#include <xswl/youdidit/web/web_dashboard.hpp>
#include <xswl/youdidit/web/web_server.hpp>

using namespace xswl::youdidit;

int main() {
    TaskPlatform platform;
    WebDashboard dashboard(&platform);
    WebServer server(&dashboard, 8080);
    
    server.start();
    // 在 http://localhost:8080 访问仪表板
    
    // ... 业务逻辑 ...
    
    server.stop();
    return 0;
}
```

### 仪表板特性

- 实时指标显示（每 3 秒自动刷新）
- 申领者状态监控
- 任务进度跟踪
- 系统事件日志
- 响应式 Web 设计

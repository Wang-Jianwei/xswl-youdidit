# xswl-youdidit 开发计划

本文档是专为 AI 编程助手设计的详细开发计划，包含清晰的模块划分、依赖关系、实现顺序和验收标准。

## 目录

- [项目概述](#项目概述)
- [技术规范](#技术规范)
- [开发阶段规划](#开发阶段规划)
- [模块详细设计](#模块详细设计)
- [开发任务清单](#开发任务清单)
- [测试策略](#测试策略)
- [注意事项与约束](#注意事项与约束)

---

## 项目概述

### 项目目标

构建一个高性能的任务代理平台库，采用赏金榜游戏机制，支持多角色任务交互与实时监控。

### 核心功能

1. **任务管理**：任务创建、发布、申领、执行、完成的完整生命周期
2. **多角色协作**：发布者、申领者、分派者的多角色交互
3. **实时监控**：Web 仪表板、事件日志、时间回放
4. **高并发支持**：线程安全设计，支持大规模并发任务处理

### 目录结构

```
xswl-youdidit/
├── include/
│   └── xswl/
│       └── youdidit/
│           ├── core/
│           │   ├── types.hpp           # 核心类型定义
│           │   ├── task.hpp            # Task 类
│           │   ├── task_builder.hpp    # TaskBuilder 类
│           │   ├── claimer.hpp         # Claimer 类
│           │   └── task_platform.hpp   # TaskPlatform 类
│           ├── web/
│           │   ├── web_dashboard.hpp   # WebDashboard 类
│           │   ├── metrics_exporter.hpp# MetricsExporter 类
│           │   ├── event_log.hpp       # EventLog 类
│           │   ├── time_replay.hpp     # TimeReplay 类
│           │   └── web_server.hpp      # WebServer 类
│           └── youdidit.hpp            # 主头文件（包含所有公开接口）
├── src/
│   ├── core/
│   │   ├── types.cpp
│   │   ├── task.cpp
│   │   ├── task_builder.cpp
│   │   ├── claimer.cpp
│   │   └── task_platform.cpp
│   └── web/
│       ├── web_dashboard.cpp
│       ├── metrics_exporter.cpp
│       ├── event_log.cpp
│       ├── time_replay.cpp
│       └── web_server.cpp
├── tests/
│   ├── unit/
│   │   ├── test_task.cpp
│   │   ├── test_claimer.cpp
│   │   ├── test_task_platform.cpp
│   │   └── test_thread_safety.cpp
│   ├── integration/
│   │   ├── test_workflow.cpp
│   │   └── test_web_api.cpp
│   └── CMakeLists.txt
├── examples/
│   ├── basic_usage.cpp
│   ├── multi_claimer.cpp
│   └── web_monitoring.cpp
├── third_party/
│   ├── tl_expected/
│   ├── tl_optional/
│   └── xswl_signals/
├── web/                    # Web 前端资源
│   ├── index.html
│   ├── css/
│   └── js/
├── CMakeLists.txt
├── README.md
├── API.md
└── docs/
    ├── WEB_API.md
    ├── WEB_MONITORING.md
    └── DEVELOPMENT_PLAN.md  # 本文件
```

---

## 技术规范

### 编译环境

| 项目 | 要求 |
|------|------|
| **C++ 标准** | C++11 |
| **编译器** | GCC 4.8+, Clang 3.4+, MSVC 2015+, MinGW |
| **构建工具** | CMake 3.10+ |
| **平台** | Windows, Linux, macOS |

### 第三方依赖

| 依赖库 | 用途 | 引入方式 |
|--------|------|----------|
| **tl::optional** | C++11 兼容的 optional 实现 | Header-only |
| **tl::expected** | 错误处理类型 | Header-only |
| **xswl-signals** | 信号槽机制 | Git submodule |
| **nlohmann/json** | JSON 序列化（可选） | Header-only |

### 编码规范

参考 [API.md](../../API.md#编程风格规范)，核心要点：

1. **私有成员函数**：使用下划线前缀 `_method_name()`
2. **引用类型**：右对齐写法 `const std::string &param`
3. **Pimpl 指针**：使用简洁的 `d` 命名
4. **noexcept**：Getter 方法标注 noexcept
5. **Getter 命名**：使用简洁命名，如 `status()` 而非 `get_status()`

---

## 开发阶段规划

### 阶段总览

```
┌─────────────────────────────────────────────────────────────────────┐
│                        开发阶段规划                                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  阶段 1: 基础设施 (Phase 1: Foundation)                             │
│  ├─ 项目结构搭建                                                    │
│  ├─ CMake 构建系统                                                  │
│  ├─ 第三方依赖集成                                                  │
│  └─ 核心类型定义                                                    │
│                                                                     │
│  阶段 2: 核心模块 (Phase 2: Core Modules)                           │
│  ├─ Task 类实现                                                     │
│  ├─ TaskBuilder 类实现                                              │
│  ├─ Claimer 类实现                                                  │
│  └─ TaskPlatform 类实现                                             │
│                                                                     │
│  阶段 3: 线程安全 (Phase 3: Thread Safety)                          │
│  ├─ 原子操作和锁机制                                                │
│  ├─ 读写锁优化                                                      │
│  └─ 并发测试                                                        │
│                                                                     │
│  阶段 4: Web 监控 (Phase 4: Web Monitoring)                         │
│  ├─ EventLog 类实现                                                 │
│  ├─ TimeReplay 类实现                                               │
│  ├─ MetricsExporter 类实现                                          │
│  ├─ WebDashboard 类实现                                             │
│  └─ WebServer 类实现                                                │
│                                                                     │
│  阶段 5: 测试与文档 (Phase 5: Testing & Documentation)              │
│  ├─ 单元测试                                                        │
│  ├─ 集成测试                                                        │
│  ├─ 示例代码                                                        │
│  └─ 使用文档                                                        │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 阶段依赖关系

```
Phase 1 ──→ Phase 2 ──→ Phase 3 ──→ Phase 4 ──→ Phase 5
   │           │           │           │
   │           ├───────────┼───────────┤
   │           │           │           │
   ▼           ▼           ▼           ▼
 types.hpp  task.hpp   locks.hpp   web/*.hpp
            claimer.hpp
            platform.hpp
```

---

## 模块详细设计

### 阶段 1：基础设施

#### 任务 1.1：项目结构初始化

**目标**：创建完整的目录结构和基础文件

**输出文件**：
- `CMakeLists.txt` - 根构建配置
- `include/xswl/youdidit/youdidit.hpp` - 主头文件
- `src/CMakeLists.txt` - 源码构建配置
- `tests/CMakeLists.txt` - 测试构建配置

**验收标准**：
- [ ] 目录结构完整
- [ ] CMake 可以成功配置（无编译目标）
- [ ] 主头文件可被包含

---

#### 任务 1.2：第三方依赖集成

**目标**：集成所有第三方 header-only 库

**依赖获取**：
```bash
# tl::optional
# 下载 https://github.com/TartanLlama/optional

# tl::expected
# 下载 https://github.com/TartanLlama/expected

# xswl-signals
git submodule add https://github.com/Wang-Jianwei/xswl-signals.git third_party/xswl_signals

# nlohmann/json (可选)
# 下载 https://github.com/nlohmann/json
```

**输出文件**：
- `third_party/tl_optional/optional.hpp`
- `third_party/tl_expected/expected.hpp`
- `third_party/xswl_signals/` (git submodule)
- `third_party/nlohmann/json.hpp` (可选)

**验收标准**：
- [ ] 所有头文件可被正确包含
- [ ] CMake 配置正确设置 include 路径
- [ ] 简单测试代码可编译通过

---

#### 任务 1.3：核心类型定义

**目标**：实现所有基础类型和枚举

**输出文件**：
- `include/xswl/youdidit/core/types.hpp`
- `src/core/types.cpp`

**类型定义清单**：

```cpp
// types.hpp 需要定义的内容

// 1. 类型别名
using TaskId = std::string;
using Timestamp = std::chrono::system_clock::time_point;

// 2. TaskStatus 枚举
enum class TaskStatus {
    Draft, Published, Claimed, Processing,
    Paused, Completed, Failed, Cancelled, Abandoned
};
// 成员方法：to_string(), from_string()

// 3. ClaimerState 结构
struct ClaimerState {
    bool online;
    bool accepting_new_tasks;
    int active_task_count;
    int max_concurrent;
    // 辅助方法：is_idle(), is_working(), is_busy(), is_paused(), is_offline()
};
// 工具函数：to_string(const ClaimerState&), claimer_state_from_string(...) 

// 4. TaskResult 结构体
struct TaskResult {
    bool success;
    std::string summary;
    std::string output;  // 输出数据（自由文本）
    tl::optional<std::string> error_message;
    
    TaskResult();
    TaskResult(bool success, const std::string &summary);
};

// 5. Error 结构体
struct Error {
    std::string message;
    int code;
    
    explicit Error(const std::string &msg, int error_code = 0);
};

// 6. 错误码常量
namespace ErrorCode {
    constexpr int SUCCESS = 0;
    constexpr int TASK_NOT_FOUND = 1001;
    constexpr int TASK_STATUS_INVALID = 1002;
    constexpr int TASK_ALREADY_CLAIMED = 1003;
    constexpr int TASK_CATEGORY_MISMATCH = 1004;
    constexpr int CLAIMER_NOT_FOUND = 2001;
    constexpr int CLAIMER_TOO_MANY_TASKS = 2002;
    constexpr int CLAIMER_ROLE_MISMATCH = 2003;
    constexpr int CLAIMER_BLOCKED = 2004;
    constexpr int CLAIMER_NOT_ALLOWED = 2005;
    constexpr int PLATFORM_QUEUE_FULL = 3001;
    constexpr int PLATFORM_NO_AVAILABLE_TASK = 3002;
}
```

**验收标准**：
- [ ] 所有类型可正确编译
- [ ] 枚举的 to_string/from_string 方法正常工作
- [ ] 错误码定义完整

---

### 阶段 2：核心模块

#### 任务 2.1：Task 类实现

**目标**：实现完整的 Task 类

**输出文件**：
- `include/xswl/youdidit/core/task.hpp`
- `src/core/task.cpp`

**实现要点**：

```cpp
// 1. Pimpl 模式结构
class Task {
public:
    // 公有接口（详见 API.md）
private:
    class Impl;
    std::unique_ptr<Impl> d;
};

// 2. 内部实现类
class Task::Impl {
public:
    // 基本属性
    TaskId id_;
    std::string title_;
    std::string description_;
    int priority_ = 0;
    std::string category_;
    std::vector<std::string> tags_;
    
    // 角色相关
    std::string publisher_id_;
    tl::optional<std::string> assignee_id_;
    tl::optional<std::string> claimer_id_;
    std::string required_role_;
    
    // 申领者限制
    std::set<std::string> allowed_claimers_;
    std::set<std::string> blocked_claimers_;
    
    // 状态（原子操作）
    std::atomic<TaskStatus> status_{TaskStatus::Draft};
    std::atomic<int> progress_{0};
    
    // 时间信息
    Timestamp created_at_;
    Timestamp published_at_;
    tl::optional<Timestamp> claimed_at_;
    tl::optional<Timestamp> started_at_;
    tl::optional<Timestamp> completed_at_;
    Timestamp deadline_;
    
    // 奖励
    int reward_points_ = 0;
    std::string reward_type_;
    
    // 结果和元数据
    tl::optional<TaskResult> result_;
    std::map<std::string, std::string> metadata_;
    
    // 业务逻辑处理函数
    TaskHandler handler_;
    
    // 信号
    xswl::signal_t<TaskStatus> sig_status_changed_;
    xswl::signal_t<int> sig_progress_updated_;
    xswl::signal_t<const TaskId&> sig_published_;
    xswl::signal_t<const TaskId&, const std::string&> sig_claimed_;
    xswl::signal_t<const TaskId&> sig_started_;
    xswl::signal_t<const TaskId&, const TaskResult&> sig_completed_;
    xswl::signal_t<const TaskId&, const std::string&> sig_failed_;
    xswl::signal_t<int, int> sig_priority_changed_;
    
    // 线程安全
    mutable std::shared_mutex data_mutex_;
    
    // 私有方法
    void _validate_status_transition(TaskStatus new_status);
    void _trigger_status_signal(TaskStatus old_status, TaskStatus new_status);
};
```

**关键方法实现逻辑**：

| 方法 | 实现要点 |
|------|---------|
| `set_status()` | 验证状态转换有效性，触发信号，线程安全 |
| `set_progress()` | 原子操作，触发进度信号 |
| `execute()` | 调用 handler，处理返回结果，更新状态 |
| `can_transition_to()` | 根据状态机规则返回是否可转换 |
| `is_claimer_allowed()` | 检查黑名单和白名单 |

**状态转换规则**：

```
Draft      → Published
Published  → Claimed, Cancelled
Claimed    → Processing, Abandoned
Processing → Paused, Completed, Failed
Paused     → Processing, Abandoned
Failed     → Published, Abandoned
Abandoned  → Published
Completed  → (终态)
Cancelled  → (终态)
```

**验收标准**：
- [ ] 所有属性的 Getter/Setter 正常工作
- [ ] 状态转换验证正确
- [ ] 信号触发正确
- [ ] execute() 方法正确调用 handler
- [ ] is_claimer_allowed() 正确实现黑白名单逻辑

---

#### 任务 2.2：TaskBuilder 类实现

**目标**：实现 Fluent API 风格的任务构建器

**输出文件**：
- `include/xswl/youdidit/core/task_builder.hpp`
- `src/core/task_builder.cpp`

**实现要点**：

```cpp
class TaskBuilder {
public:
    TaskBuilder();
    explicit TaskBuilder(TaskPlatform* platform);
    
    // Fluent API - 每个方法返回 *this
    TaskBuilder &title(const std::string &title);
    TaskBuilder &description(const std::string &desc);
    TaskBuilder &priority(int priority);
    TaskBuilder &category(const std::string &category);
    TaskBuilder &tag(const std::string &tag);
    TaskBuilder &tags(const std::vector<std::string> &tags);
    
    TaskBuilder &publisher_id(const std::string &id);
    TaskBuilder &assignee_id(const std::string &id);
    TaskBuilder &required_role(const std::string &role);
    
    TaskBuilder &allowed_claimer(const std::string &claimer_id);
    TaskBuilder &allowed_claimers(const std::set<std::string> &ids);
    TaskBuilder &blocked_claimer(const std::string &claimer_id);
    TaskBuilder &blocked_claimers(const std::set<std::string> &ids);
    
    TaskBuilder &deadline(const Timestamp &deadline);
    TaskBuilder &deadline_in(const std::chrono::seconds &duration);
    
    TaskBuilder &reward_points(int points);
    TaskBuilder &reward_type(const std::string &type);
    
    TaskBuilder &metadata(const std::string &key, const std::string &value);
    TaskBuilder &metadata(const std::map<std::string, std::string> &data);
    
    TaskBuilder &handler(Task::TaskHandler handler);
    
    // 构建方法
    std::shared_ptr<Task> build();
    std::shared_ptr<Task> build_and_publish();
    
    // 工具方法
    TaskBuilder &reset();
    bool is_valid() const;
    std::vector<std::string> validation_errors() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

**验证规则实现**：

```cpp
std::vector<std::string> TaskBuilder::validation_errors() const {
    std::vector<std::string> errors;
    
    if (d->title_.empty()) {
        errors.push_back("Title is required");
    }
    if (d->priority_ < 1 || d->priority_ > 10) {
        errors.push_back("Priority must be between 1 and 10");
    }
    if (d->deadline_ != Timestamp{} && d->deadline_ <= std::chrono::system_clock::now()) {
        errors.push_back("Deadline must be in the future");
    }
    if (!d->handler_) {
        errors.push_back("Handler is required");
    }
    if (d->reward_points_ < 0) {
        errors.push_back("Reward points must be non-negative");
    }
    
    return errors;
}
```

**验收标准**：
- [ ] Fluent API 链式调用正常
- [ ] 所有验证规则正确实现
- [ ] build() 生成正确配置的 Task 对象
- [ ] build_and_publish() 正确发布到平台
- [ ] reset() 正确清空所有配置

---

#### 任务 2.3：Claimer 类实现

**目标**：实现任务申领者类

**输出文件**：
- `include/xswl/youdidit/core/claimer.hpp`
- `src/core/claimer.cpp`

**实现要点**：

```cpp
class Claimer : public std::enable_shared_from_this<Claimer> {
public:
    Claimer(const std::string &id, const std::string &name);
    ~Claimer() noexcept;
    
    // 禁止拷贝，允许移动
    Claimer(const Claimer &) = delete;
    Claimer &operator=(const Claimer &) = delete;
    Claimer(Claimer &&other) noexcept;
    Claimer &operator=(Claimer &&other) noexcept;
    
    // ... 完整 API 见 API.md
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

**核心方法实现逻辑**：

| 方法 | 实现要点 |
|------|---------|
| `claim_task()` | 验证权限 → 检查并发数 → 更新任务状态 → 触发信号 |
| `claim_next_task()` | 从平台获取优先级最高任务 → 执行 claim_task() |
| `claim_matching_task()` | 计算匹配度得分 → 选最高分任务 → 执行 claim_task() |
| `claim_tasks_to_capacity()` | 循环调用 claim_next_task() 直到达到最大并发 |
| `execute_task()` | 开始任务 → 调用 task.execute() → 处理结果 |
| `complete_task()` | 更新任务状态 → 更新统计 → 触发信号 |
| `can_claim_more()` | 比较 active_task_count 和 max_concurrent_tasks |

**申领权限检查流程**：

```cpp
tl::expected<void, Error> Claimer::Impl::_check_claim_permission(
    const std::shared_ptr<Task> &task
) const {
    // 1. 检查黑名单
    if (task->blocked_claimers().count(id_) > 0) {
        return tl::unexpected(Error("Claimer is blocked by publisher", 
                                    ErrorCode::CLAIMER_BLOCKED));
    }
    
    // 2. 检查白名单（如果有）
    auto allowed = task->allowed_claimers();
    if (!allowed.empty() && allowed.count(id_) == 0) {
        return tl::unexpected(Error("Claimer is not in allowed list", 
                                    ErrorCode::CLAIMER_NOT_ALLOWED));
    }
    
    // 3. 检查角色
    auto required = task->required_role();
    if (!required.empty() && required != role_) {
        return tl::unexpected(Error("Role mismatch", 
                                    ErrorCode::CLAIMER_ROLE_MISMATCH));
    }
    
    // 4. 检查分类（严格模式）
    if (strict_category_matching_) {
        auto task_category = task->category();
        if (!task_category.empty() && categories_.count(task_category) == 0) {
            return tl::unexpected(Error("Category mismatch", 
                                        ErrorCode::TASK_CATEGORY_MISMATCH));
        }
    }
    
    return {};
}
```

**验收标准**：
- [ ] 四种申领方式全部正确实现
- [ ] 权限检查逻辑完整（黑名单 > 白名单 > 角色 > 分类）
- [ ] 并发任务数限制正确
- [ ] 统计信息正确更新
- [ ] 所有信号正确触发

---

#### 任务 2.4：TaskPlatform 类实现

**目标**：实现任务平台核心类

**输出文件**：
- `include/xswl/youdidit/core/task_platform.hpp`
- `src/core/task_platform.cpp`

**实现要点**：

```cpp
class TaskPlatform : public std::enable_shared_from_this<TaskPlatform> {
public:
    TaskPlatform();
    explicit TaskPlatform(const std::string &platform_id);
    ~TaskPlatform();
    
    // 禁止拷贝
    TaskPlatform(const TaskPlatform &) = delete;
    TaskPlatform &operator=(const TaskPlatform &) = delete;
    
    // ... 完整 API 见 API.md
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

**内部数据结构**：

```cpp
class TaskPlatform::Impl {
public:
    std::string platform_id_;
    std::string name_;
    
    // 任务管理 - 使用 shared_mutex 保护
    mutable std::shared_mutex tasks_mutex_;
    std::unordered_map<TaskId, std::shared_ptr<Task>> tasks_;
    
    // 优先级队列（已发布任务）- 使用 mutex 保护
    mutable std::mutex queue_mutex_;
    std::priority_queue<
        std::shared_ptr<Task>,
        std::vector<std::shared_ptr<Task>>,
        TaskPriorityComparator
    > published_queue_;
    
    // 申领者管理 - 使用 shared_mutex 保护
    mutable std::shared_mutex claimers_mutex_;
    std::unordered_map<std::string, std::shared_ptr<Claimer>> claimers_;
    
    // 配置
    std::string log_file_path_;
    size_t max_queue_size_ = 10000;
    
    // 统计
    Timestamp start_time_;
    std::atomic<size_t> total_completed_{0};
    std::atomic<size_t> total_failed_{0};
    
    // 信号
    xswl::signal_t<const std::shared_ptr<Task>&> sig_task_published_;
    xswl::signal_t<const std::shared_ptr<Task>&> sig_task_claimed_;
    // ... 其他信号
    
    // 私有方法
    void _add_to_published_queue(const std::shared_ptr<Task> &task);
    void _remove_from_published_queue(const TaskId &task_id);
    void _update_statistics();
};
```

**验收标准**：
- [ ] 任务发布和管理正确
- [ ] 申领者注册和管理正确
- [ ] 任务查询和过滤正确
- [ ] 统计信息正确计算
- [ ] 所有信号正确触发

---

### 阶段 3：线程安全

#### 任务 3.1：原子操作实现

**目标**：为关键数据添加原子操作支持

**涉及文件**：
- `src/core/task.cpp`
- `src/core/claimer.cpp`

**原子操作列表**：

| 类 | 属性 | 类型 |
|----|------|------|
| Task | status | `std::atomic<TaskStatus>` |
| Task | progress | `std::atomic<int>` |
| Claimer | status | `ClaimerState` |
| Claimer | active_task_count | `std::atomic<int>` |
| TaskPlatform | total_completed | `std::atomic<size_t>` |
| TaskPlatform | total_failed | `std::atomic<size_t>` |

**实现示例**：

```cpp
// Task::set_progress() - 原子操作
Task &Task::set_progress(int progress) {
    int old_progress = d->progress_.exchange(progress, std::memory_order_acq_rel);
    if (old_progress != progress) {
        d->sig_progress_updated_.emit(progress);
    }
    return *this;
}

// Task::progress() - 原子读取
int Task::progress() const noexcept {
    return d->progress_.load(std::memory_order_acquire);
}
```

**验收标准**：
- [ ] 所有原子操作正确使用 memory_order
- [ ] 原子类型选择正确

---

#### 任务 3.2：读写锁实现

**目标**：为容器类数据添加读写锁保护

**涉及文件**：
- `src/core/task.cpp`
- `src/core/claimer.cpp`
- `src/core/task_platform.cpp`

**读写锁使用场景**：

| 类 | 保护数据 | 读操作 | 写操作 |
|----|---------|--------|--------|
| Task | metadata | `metadata()`, `get_metadata()` | `set_metadata()` |
| Claimer | claimed_tasks | `claimed_tasks()`, `get_active_tasks()` | claim/complete |
| TaskPlatform | tasks | `get_task()`, 查询方法 | `publish_task()` |
| TaskPlatform | claimers | `get_claimer()` | `register_claimer()` |

**实现示例**：

```cpp
// 读操作 - 共享锁
std::map<std::string, std::string> Task::metadata() const {
    std::shared_lock<std::shared_mutex> lock(d->data_mutex_);
    return d->metadata_;  // 返回副本
}

// 写操作 - 独占锁
Task &Task::set_metadata(const std::string &key, const std::string &value) {
    std::unique_lock<std::shared_mutex> lock(d->data_mutex_);
    d->metadata_[key] = value;
    return *this;
}
```

**验收标准**：
- [ ] 读操作使用 shared_lock
- [ ] 写操作使用 unique_lock
- [ ] 无死锁风险（遵循加锁顺序：platform > task > data）

---

#### 任务 3.3：并发安全的任务申领

**目标**：确保多申领者并发申领同一任务时只有一个成功

**涉及文件**：
- `src/core/task_platform.cpp`
- `src/core/claimer.cpp`

**实现方案**：

```cpp
tl::expected<std::shared_ptr<Task>, Error> TaskPlatform::Impl::_try_claim_task(
    const TaskId &task_id,
    const std::shared_ptr<Claimer> &claimer
) {
    // 1. 获取任务（共享锁）
    std::shared_ptr<Task> task;
    {
        std::shared_lock<std::shared_mutex> lock(tasks_mutex_);
        auto it = tasks_.find(task_id);
        if (it == tasks_.end()) {
            return tl::unexpected(Error("Task not found", ErrorCode::TASK_NOT_FOUND));
        }
        task = it->second;
    }
    
    // 2. 尝试原子更新任务状态
    TaskStatus expected = TaskStatus::Published;
    if (!task->_try_set_status_atomic(expected, TaskStatus::Claimed)) {
        return tl::unexpected(Error("Task already claimed", 
                                    ErrorCode::TASK_ALREADY_CLAIMED));
    }
    
    // 3. 更新任务的申领者信息
    task->_set_claimer_id(claimer->id());
    
    // 4. 触发信号
    sig_task_claimed_.emit(task);
    
    return task;
}
```

**验收标准**：
- [ ] 多线程并发申领同一任务，只有一个成功
- [ ] 申领失败的线程收到正确的错误信息
- [ ] 无竞态条件

---

### 阶段 4：Web 监控

#### 任务 4.1：EventLog 类实现

**目标**：实现事件日志记录与查询

**输出文件**：
- `include/xswl/youdidit/web/event_log.hpp`
- `web/src/event_log.cpp`

**实现要点**：

```cpp
class EventLog {
public:
    enum class EventType {
        TaskPublished, TaskClaimed, TaskStarted,
        TaskProgressUpdated, TaskCompleted, TaskFailed,
        TaskAbandoned, PriorityChanged,
        ClaimerRegistered, ClaimerStateChanged
    };
    
    struct EventRecord {
        EventType type;
        Timestamp timestamp;
        std::string source_id;
        std::map<std::string, std::string> data;
        std::string description;
    };
    
    void log_event(const EventRecord &record);  // 线程安全
    
    std::vector<EventRecord> get_events(
        const Timestamp &start_time,
        const Timestamp &end_time
    ) const;  // 线程安全
    
    std::vector<EventRecord> get_events_by_source(
        const std::string &source_id
    ) const;  // 线程安全
    
    std::vector<EventRecord> get_events_by_type(
        EventType event_type
    ) const;  // 线程安全
    
    std::string export_as_json(const Timestamp &start, const Timestamp &end) const;
    std::string export_as_csv(const Timestamp &start, const Timestamp &end) const;
    
    void cleanup_events_before(const Timestamp &before_time);
    
private:
    mutable std::shared_mutex events_mutex_;
    std::deque<EventRecord> events_;
    std::unordered_multimap<std::string, size_t> source_index_;
};
```

**验收标准**：
- [ ] 事件记录线程安全
- [ ] 查询方法正确过滤
- [ ] JSON/CSV 导出格式正确
- [ ] 清理方法正确删除旧事件

---

#### 任务 4.2：TimeReplay 类实现

**目标**：实现时间回放功能

**输出文件**：
- `include/xswl/youdidit/web/time_replay.hpp`
- `web/src/time_replay.cpp`

**实现要点**：

```cpp
class TimeReplay {
public:
    explicit TimeReplay(EventLog* event_log, TaskPlatform* platform);
    
    // 获取指定时刻的系统快照
    std::string get_snapshot_at(const Timestamp &timestamp) const;
    
    // 获取时间段内的事件序列
    std::vector<EventLog::EventRecord> get_events_between(
        const Timestamp &start_time,
        const Timestamp &end_time
    ) const;
    
    // 获取指定时刻的任务状态
    std::string get_task_state_at(
        const TaskId &task_id,
        const Timestamp &timestamp
    ) const;
    
    // 获取指定时刻的申领者状态
    std::string get_claimer_state_at(
        const std::string &claimer_id,
        const Timestamp &timestamp
    ) const;
    
    // 生成状态演化轨迹
    std::string generate_state_trace(
        const Timestamp &start_time,
        const Timestamp &end_time,
        int snapshot_interval_ms = 100
    ) const;
    
private:
    EventLog* event_log_;
    TaskPlatform* platform_;
    
    // 从事件序列重建状态
    std::string _reconstruct_state_at(const Timestamp &timestamp) const;
};
```

**验收标准**：
- [ ] 快照正确反映指定时刻的状态
- [ ] 状态轨迹正确生成
- [ ] 性能可接受（大量事件时）

---

#### 任务 4.3：MetricsExporter 类实现

**目标**：实现跨进程监控数据导出

**输出文件**：
- `include/xswl/youdidit/web/metrics_exporter.hpp`
- `web/src/metrics_exporter.cpp`

**实现要点**：

```cpp
class MetricsExporter {
public:
    explicit MetricsExporter(TaskPlatform* platform);
    ~MetricsExporter();
    
    void start_server(int port = 9090);
    void stop_server();
    bool is_running() const noexcept;
    
    MetricsExporter &set_update_interval(int milliseconds);
    MetricsExporter &enable_websocket(bool enable = true);
    MetricsExporter &set_cors_origin(const std::string &origin);
    
    // 手动获取数据（JSON 格式）
    std::string get_metrics_json() const;
    std::string get_tasks_json() const;
    std::string get_claimers_json() const;
    std::string get_events_json(int limit = 100) const;
    
    // Prometheus 格式导出
    std::string export_prometheus_metrics() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

**HTTP 端点**：

| 端点 | 方法 | 返回格式 |
|------|------|---------|
| `/metrics` | GET | JSON |
| `/tasks` | GET | JSON |
| `/tasks/{id}` | GET | JSON |
| `/claimers` | GET | JSON |
| `/claimers/{id}` | GET | JSON |
| `/events` | GET | JSON |
| `/prometheus` | GET | Prometheus 格式 |

**验收标准**：
- [ ] HTTP 服务器正常启动/停止
- [ ] 所有端点返回正确格式的数据
- [ ] Prometheus 格式正确
- [ ] WebSocket 实时推送正常（如启用）

---

#### 任务 4.4：WebDashboard 类实现

**目标**：实现 Web 仪表板核心类

**输出文件**：
- `include/xswl/youdidit/web/web_dashboard.hpp`
- `web/src/web_dashboard.cpp`

**实现要点**：

```cpp
class WebDashboard {
public:
    // 同进程模式
    explicit WebDashboard(TaskPlatform* platform);
    
    // 远程模式
    explicit WebDashboard(const std::string &metrics_endpoint);
    
    // 多平台聚合模式
    explicit WebDashboard(const std::vector<std::string> &endpoints);
    
    ~WebDashboard();
    
    void start_server(int port = 8080);
    void stop_server();
    bool is_running() const noexcept;
    
    // 配置（Fluent API）
    WebDashboard &set_update_interval(int milliseconds);
    WebDashboard &set_log_file_path(const std::string &path);
    WebDashboard &set_max_event_history(size_t max_events);
    WebDashboard &enable_https(const std::string &cert_path, const std::string &key_path);
    
    // 数据访问
    using DashboardMetrics = TaskPlatform::PlatformStatistics;
    DashboardMetrics get_metrics() const;
    std::string get_dashboard_data() const;
    
    // 任务和申领者信息
    struct TaskSummary { /* ... */ };
    struct ClaimerSummary { /* ... */ };
    
    std::vector<TaskSummary> get_tasks_summary() const;
    std::vector<ClaimerSummary> get_claimers_summary() const;
    
    // 时间回放
    std::shared_ptr<TimeReplay> get_time_replay() const;
    
    // 事件日志
    std::vector<std::string> get_event_logs(int limit = 100, int offset = 0) const;
    
    // 性能分析
    struct PerformanceAnalysis { /* ... */ };
    PerformanceAnalysis analyze_performance(
        const Timestamp &start_time,
        const Timestamp &end_time
    ) const;
    
    // 数据导出
    std::string export_as_json(const Timestamp &start, const Timestamp &end) const;
    std::string export_as_csv(const Timestamp &start, const Timestamp &end) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

**验收标准**：
- [ ] 三种构造模式都正常工作
- [ ] Web 服务器正常启动/停止
- [ ] 所有数据访问方法正确
- [ ] 时间回放功能正常
- [ ] 数据导出格式正确

---

#### 任务 4.5：WebServer 类实现

**目标**：实现内置 HTTP 服务器

**输出文件**：
- `include/xswl/youdidit/web/web_server.hpp`
- `web/src/web_server.cpp`

**实现方案选择**：

| 方案 | 优点 | 缺点 | 推荐度 |
|------|------|------|--------|
| **自实现简单 HTTP** | 无额外依赖 | 功能有限 | ★★★ |
| **cpp-httplib** | 轻量 header-only | 功能完整 | ★★★★★ |
| **Boost.Beast** | 性能优秀 | 依赖 Boost | ★★★ |

**推荐使用 cpp-httplib**（header-only，MIT 许可）：

```cpp
class WebServer {
public:
    WebServer(WebDashboard* dashboard, int port);
    ~WebServer();
    
    void start();
    void stop();
    bool is_running() const;
    
    void set_port(int port);
    int get_port() const;
    
private:
    WebDashboard* dashboard_;
    int port_;
    std::unique_ptr<httplib::Server> server_;
    std::thread server_thread_;
    
    void _setup_routes();
    void _serve_static_files();
};
```

**验收标准**：
- [ ] HTTP 服务器正常启动/停止
- [ ] REST API 端点正确响应
- [ ] 静态文件服务正常
- [ ] CORS 头正确设置

---

### 阶段 5：测试与文档

#### 任务 5.1：单元测试

**目标**：为所有核心类编写单元测试

**输出文件**：
- `tests/unit/test_types.cpp`
- `tests/unit/test_task.cpp`
- `tests/unit/test_task_builder.cpp`
- `tests/unit/test_claimer.cpp`
- `tests/unit/test_task_platform.cpp`

**测试框架**：推荐使用 Google Test 或 Catch2

**测试用例清单**：

```cpp
// test_task.cpp
TEST(Task, DefaultConstruction)
TEST(Task, SetAndGetProperties)
TEST(Task, StatusTransition_ValidPath)
TEST(Task, StatusTransition_InvalidPath)
TEST(Task, ProgressUpdate)
TEST(Task, SignalEmission)
TEST(Task, ClaimerRestriction_Allowed)
TEST(Task, ClaimerRestriction_Blocked)
TEST(Task, Execute_Success)
TEST(Task, Execute_Failure)

// test_claimer.cpp
TEST(Claimer, DefaultConstruction)
TEST(Claimer, SetAndGetProperties)
TEST(Claimer, ClaimTask_Success)
TEST(Claimer, ClaimTask_TooManyTasks)
TEST(Claimer, ClaimTask_RoleMismatch)
TEST(Claimer, ClaimTask_CategoryMismatch)
TEST(Claimer, ClaimNextTask)
TEST(Claimer, ClaimMatchingTask)
TEST(Claimer, ClaimTasksToCapacity)
TEST(Claimer, ExecuteTask)
TEST(Claimer, StatisticsUpdate)

// test_task_platform.cpp
TEST(TaskPlatform, PublishTask)
TEST(TaskPlatform, GetTask)
TEST(TaskPlatform, TaskFilter)
TEST(TaskPlatform, RegisterClaimer)
TEST(TaskPlatform, AssignTask)
TEST(TaskPlatform, Statistics)
```

**验收标准**：
- [ ] 所有核心功能有对应测试
- [ ] 边界条件有测试覆盖
- [ ] 错误路径有测试覆盖
- [ ] 测试通过率 100%

---

#### 任务 5.2：线程安全测试

**目标**：验证多线程环境下的正确性

**输出文件**：
- `tests/unit/test_thread_safety.cpp`

**测试场景**：

```cpp
// 并发申领同一任务
TEST(ThreadSafety, ConcurrentClaimSameTask) {
    // 多个线程同时尝试申领同一任务
    // 验证只有一个成功，其他失败
}

// 并发修改任务状态
TEST(ThreadSafety, ConcurrentStatusUpdate) {
    // 多个线程同时修改任务状态
    // 验证状态转换的原子性
}

// 并发发布和申领
TEST(ThreadSafety, ConcurrentPublishAndClaim) {
    // 一边发布任务，一边申领
    // 验证无数据竞争
}

// 并发更新进度
TEST(ThreadSafety, ConcurrentProgressUpdate) {
    // 多个线程频繁更新进度
    // 验证原子操作正确
}
```

**验收标准**：
- [ ] 无竞态条件
- [ ] 无死锁
- [ ] ThreadSanitizer 无警告

---

#### 任务 5.3：集成测试

**目标**：测试完整的工作流程

**输出文件**：
- `tests/integration/test_workflow.cpp`
- `tests/integration/test_web_api.cpp`

**测试场景**：

```cpp
// 完整任务生命周期
TEST(Integration, CompleteTaskLifecycle) {
    // 创建平台 → 创建任务 → 发布 → 申领 → 执行 → 完成
    // 验证每个阶段的状态和信号
}

// 多申领者协作
TEST(Integration, MultiClaimerCollaboration) {
    // 多个申领者处理多个任务
    // 验证负载均衡和公平调度
}

// Web API 端到端测试
TEST(Integration, WebApiEndToEnd) {
    // 启动 WebDashboard → 发送 HTTP 请求 → 验证响应
}
```

**验收标准**：
- [ ] 完整工作流程正常
- [ ] 多角色协作正确
- [ ] Web API 响应正确

---

#### 任务 5.4：示例代码

**目标**：编写展示库用法的示例代码

**输出文件**：
- `examples/basic_usage.cpp` - 基本用法
- `examples/multi_claimer.cpp` - 多申领者场景
- `examples/web_monitoring.cpp` - Web 监控示例

**示例 1：基本用法**

```cpp
// examples/basic_usage.cpp
#include <xswl/youdidit/youdidit.hpp>
#include <iostream>

using namespace xswl::youdidit;

int main() {
    // 创建平台
    auto platform = std::make_shared<TaskPlatform>("demo-platform");
    
    // 创建并发布任务
    auto task = platform->task_builder()
        .title("数据处理任务")
        .priority(5)
        .category("data_processing")
        .handler([](Task &t, const std::string &input) {
            std::cout << "处理输入: " << input << std::endl;
            t.set_progress(50);
            // 模拟处理
            t.set_progress(100);
            return TaskResult("处理完成");
        })
        .build_and_publish();
    
    std::cout << "任务已发布: " << task->id() << std::endl;
    
    // 创建申领者
    auto claimer = std::make_shared<Claimer>("worker-001", "Alice");
    claimer->add_category("data_processing")
           .set_max_concurrent(5);
    platform->register_claimer(claimer);
    
    // 申领并执行任务
    auto result = claimer->claim_task(task->id());
    if (result) {
        claimer->execute_task(task->id(), "/data/input.csv");
        std::cout << "任务执行完成" << std::endl;
    }
    
    // 查看统计
    auto stats = platform->get_statistics();
    std::cout << "已完成任务: " << stats.completed_tasks << std::endl;
    
    return 0;
}
```

**验收标准**：
- [ ] 示例代码可编译运行
- [ ] 代码注释清晰
- [ ] 覆盖主要使用场景

---

## 开发任务清单

以下是按优先级排序的开发任务清单，适合 AI 编程助手逐步执行：

### 高优先级（必须完成）

| ID | 任务 | 依赖 | 预估工作量 |
|----|------|------|-----------|
| T1.1 | 项目结构初始化 | 无 | 小 |
| T1.2 | 第三方依赖集成 | T1.1 | 小 |
| T1.3 | 核心类型定义 | T1.2 | 中 |
| T2.1 | Task 类实现 | T1.3 | 大 |
| T2.2 | TaskBuilder 类实现 | T2.1 | 中 |
| T2.3 | Claimer 类实现 | T2.1 | 大 |
| T2.4 | TaskPlatform 类实现 | T2.1, T2.3 | 大 |
| T3.1 | 原子操作实现 | T2.* | 中 |
| T3.2 | 读写锁实现 | T2.* | 中 |
| T3.3 | 并发安全申领 | T3.1, T3.2 | 中 |
| T5.1 | 单元测试 | T2.*, T3.* | 大 |

### 中优先级（推荐完成）

| ID | 任务 | 依赖 | 预估工作量 |
|----|------|------|-----------|
| T4.1 | EventLog 类实现 | T2.4 | 中 |
| T4.2 | TimeReplay 类实现 | T4.1 | 中 |
| T4.3 | MetricsExporter 类实现 | T2.4 | 中 |
| T4.4 | WebDashboard 类实现 | T4.1, T4.2, T4.3 | 大 |
| T4.5 | WebServer 类实现 | T4.4 | 中 |
| T5.2 | 线程安全测试 | T3.* | 中 |
| T5.3 | 集成测试 | T4.* | 中 |

### 低优先级（可选完成）

| ID | 任务 | 依赖 | 预估工作量 |
|----|------|------|-----------|
| T5.4 | 示例代码 | T2.*, T4.* | 小 |
| T6.1 | Web 前端开发 | T4.5 | 大 |
| T6.2 | 性能优化 | 全部 | 中 |
| T6.3 | 文档完善 | 全部 | 小 |

---

## 测试策略

### 测试金字塔

```
                    ┌───────────┐
                    │   E2E     │  ← 少量端到端测试
                    │   Tests   │
                    ├───────────┤
                    │Integration│  ← 中量集成测试
                    │   Tests   │
                    ├───────────┤
                    │   Unit    │  ← 大量单元测试
                    │   Tests   │
                    └───────────┘
```

### 测试覆盖率目标

| 模块 | 目标覆盖率 |
|------|-----------|
| 核心类型 | ≥ 90% |
| Task 类 | ≥ 85% |
| Claimer 类 | ≥ 85% |
| TaskPlatform 类 | ≥ 80% |
| Web 模块 | ≥ 70% |

### 关键测试场景

1. **状态转换测试**：验证所有有效和无效的状态转换
2. **权限检查测试**：验证黑名单、白名单、角色、分类匹配
3. **并发测试**：验证多线程下的正确性
4. **边界测试**：验证边界条件（空值、极大值等）
5. **错误处理测试**：验证错误路径和错误信息

---

## 注意事项与约束

### 编码注意事项

1. **Pimpl 模式**：所有公开类使用 Pimpl 模式隐藏实现细节
2. **内存管理**：使用智能指针管理所有动态分配的对象
3. **异常安全**：保证基本异常安全，关键操作保证强异常安全
4. **命名规范**：严格遵循项目编码规范

### 性能约束

1. **锁竞争**：使用细粒度锁，避免长时间持锁
2. **内存分配**：减少频繁的小内存分配
3. **信号触发**：信号回调应该快速返回，避免阻塞

### 兼容性约束

1. **C++11 兼容**：不使用 C++14 及以上特性
2. **跨平台**：代码应在 Windows、Linux、macOS 上正常编译运行
3. **编译器兼容**：支持 GCC、Clang、MSVC、MinGW

### AI 编程助手指南

1. **增量开发**：每次只实现一个任务，确保编译通过后再继续
2. **测试驱动**：先写测试，再实现功能
3. **文档同步**：代码变更时同步更新文档
4. **代码审查**：关键代码需要人工审查

---

## 附录

### A. CMakeLists.txt 模板

```cmake
cmake_minimum_required(VERSION 3.10)
project(xswl-youdidit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 第三方依赖
include_directories(${CMAKE_SOURCE_DIR}/third_party/tl_optional)
include_directories(${CMAKE_SOURCE_DIR}/third_party/tl_expected)
include_directories(${CMAKE_SOURCE_DIR}/third_party/xswl_signals/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann)

# 项目头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
file(GLOB_RECURSE CORE_SOURCES src/core/*.cpp)
file(GLOB_RECURSE WEB_SOURCES src/web/*.cpp)

# 核心库
add_library(youdidit_core ${CORE_SOURCES})

# Web 模块（可选）
option(BUILD_WEB_MODULE "Build web monitoring module" ON)
if(BUILD_WEB_MODULE)
    add_library(youdidit_web ${WEB_SOURCES})
    target_link_libraries(youdidit_web youdidit_core)
endif()

# 测试
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
```

### B. 头文件模板

```cpp
// include/xswl/youdidit/core/task.hpp
#ifndef XSWL_YOUDIDIT_CORE_TASK_HPP
#define XSWL_YOUDIDIT_CORE_TASK_HPP

#include <xswl/youdidit/core/types.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>

namespace xswl {
namespace youdidit {

class Task {
public:
    // 类型定义
    using TaskHandler = std::function<TaskResult(
        Task &task,
        const std::string &input
    )>;
    
    // 构造与析构
    Task();
    explicit Task(const TaskId &id);
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    Task(Task &&other) noexcept;
    Task &operator=(Task &&other) noexcept;
    ~Task() noexcept;
    
    // ... 公有接口
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_CORE_TASK_HPP
```

---

## AI 编程助手使用指南

本节专门为 AI 编程助手（如 GitHub Copilot、Claude 等）设计，提供明确的开发指令和上下文信息。

### 开发原则

1. **增量实现**：每次只实现一个任务，确保编译通过
2. **测试优先**：先编写测试用例，再实现功能
3. **文档同步**：代码变更时同步更新相关文档
4. **遵循规范**：严格遵循项目编码规范

### 任务执行模板

当 AI 编程助手执行任务时，请按以下格式工作：

```
## 执行任务：[任务ID] [任务名称]

### 前置检查
- [ ] 依赖任务已完成
- [ ] 相关文档已阅读
- [ ] 编码规范已理解

### 实现步骤
1. 创建必要的文件
2. 实现类/函数定义
3. 编写单元测试
4. 验证编译通过
5. 运行测试确认

### 输出文件
- file1.hpp
- file1.cpp
- test_file1.cpp

### 验收检查
- [ ] 代码符合编码规范
- [ ] 所有测试通过
- [ ] 无编译警告
```

### 快速开始指令

以下是常用的开发指令，AI 助手可直接执行：

#### 指令 1：初始化项目结构

```
请执行任务 T1.1 项目结构初始化：
1. 创建完整的目录结构（include/, src/, tests/, examples/, third_party/）
2. 创建根 CMakeLists.txt
3. 创建 src/CMakeLists.txt 和 tests/CMakeLists.txt
4. 创建主头文件 include/xswl/youdidit/youdidit.hpp
```

#### 指令 2：实现核心类型

```
请执行任务 T1.3 核心类型定义：
1. 创建 include/xswl/youdidit/core/types.hpp
2. 定义 TaskId, Timestamp 类型别名
3. 定义 TaskStatus, ClaimerState（结构）与工具函数（to_string/claimer_state_from_string）
4. 定义 TaskResult, Error 结构体
5. 定义错误码常量
6. 创建 src/core/types.cpp 实现文件
7. 创建 tests/unit/test_types.cpp 测试文件
```

#### 指令 3：实现 Task 类

```
请执行任务 T2.1 Task 类实现：
1. 创建 include/xswl/youdidit/core/task.hpp
2. 使用 Pimpl 模式定义 Task 类
3. 实现所有属性的 Getter/Setter（遵循命名规范）
4. 实现状态转换验证逻辑
5. 实现信号槽接口
6. 实现 execute() 方法
7. 创建 src/core/task.cpp 实现文件
8. 创建 tests/unit/test_task.cpp 测试文件

注意事项：
- 私有方法使用下划线前缀
- Pimpl 指针命名为 d
- Getter 使用简洁命名（如 status() 而非 get_status()）
- 标注 noexcept 的方法需要保证不抛出异常
```

#### 指令 4：实现 Claimer 类

```
请执行任务 T2.3 Claimer 类实现：
1. 创建 include/xswl/youdidit/core/claimer.hpp
2. 继承 std::enable_shared_from_this<Claimer>
3. 使用 Pimpl 模式
4. 实现四种申领方式：claim_task(), claim_next_task(), 
   claim_matching_task(), claim_tasks_to_capacity()
5. 实现权限检查逻辑（黑名单 > 白名单 > 角色 > 分类）
6. 实现 execute_task() 和任务控制方法
7. 创建 src/core/claimer.cpp 实现文件
8. 创建 tests/unit/test_claimer.cpp 测试文件
```

#### 指令 5：实现 TaskPlatform 类

```
请执行任务 T2.4 TaskPlatform 类实现：
1. 创建 include/xswl/youdidit/core/task_platform.hpp
2. 继承 std::enable_shared_from_this<TaskPlatform>
3. 使用 Pimpl 模式
4. 实现任务管理（发布、查询、过滤）
5. 实现申领者管理（注册、查询）
6. 实现优先级队列
7. 实现统计信息计算
8. 创建 src/core/task_platform.cpp 实现文件
9. 创建 tests/unit/test_task_platform.cpp 测试文件
```

### 代码模板

#### 类定义模板（头文件）

```cpp
// include/xswl/youdidit/core/example.hpp
#ifndef XSWL_YOUDIDIT_CORE_EXAMPLE_HPP
#define XSWL_YOUDIDIT_CORE_EXAMPLE_HPP

#include <xswl/youdidit/core/types.hpp>
#include <memory>
#include <string>

namespace xswl {
namespace youdidit {

class Example {
public:
    // ========== 构造与析构 ==========
    Example();
    explicit Example(const std::string &id);
    ~Example() noexcept;
    
    // 禁止拷贝
    Example(const Example &) = delete;
    Example &operator=(const Example &) = delete;
    
    // 允许移动
    Example(Example &&other) noexcept;
    Example &operator=(Example &&other) noexcept;
    
    // ========== Getter 方法 (noexcept) ==========
    const std::string &id() const noexcept;
    const std::string &name() const noexcept;
    
    // ========== Setter 方法 (Fluent API) ==========
    Example &set_name(const std::string &name);
    
    // ========== 业务方法 ==========
    tl::expected<void, Error> do_something(const std::string &param);
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
    
    // 私有方法（下划线前缀）
    void _validate_input(const std::string &input);
    bool _is_valid_state() const noexcept;
};

} // namespace youdidit
} // namespace xswl

#endif // XSWL_YOUDIDIT_CORE_EXAMPLE_HPP
```

#### 类实现模板（源文件）

```cpp
// src/core/example.cpp
#include <xswl/youdidit/core/example.hpp>
#include <mutex>
#include <shared_mutex>

namespace xswl {
namespace youdidit {

// ========== 内部实现类 ==========
class Example::Impl {
public:
    std::string id_;
    std::string name_;
    
    mutable std::shared_mutex data_mutex_;
    
    Impl() = default;
    explicit Impl(const std::string &id) : id_(id) {}
};

// ========== 构造与析构 ==========
Example::Example() : d(std::make_unique<Impl>()) {}

Example::Example(const std::string &id) : d(std::make_unique<Impl>(id)) {}

Example::~Example() noexcept = default;

Example::Example(Example &&other) noexcept = default;
Example &Example::operator=(Example &&other) noexcept = default;

// ========== Getter 方法 ==========
const std::string &Example::id() const noexcept {
    return d->id_;
}

const std::string &Example::name() const noexcept {
    std::shared_lock<std::shared_mutex> lock(d->data_mutex_);
    return d->name_;
}

// ========== Setter 方法 ==========
Example &Example::set_name(const std::string &name) {
    std::unique_lock<std::shared_mutex> lock(d->data_mutex_);
    d->name_ = name;
    return *this;
}

// ========== 业务方法 ==========
tl::expected<void, Error> Example::do_something(const std::string &param) {
    _validate_input(param);
    
    if (!_is_valid_state()) {
        return tl::unexpected(Error("Invalid state", 1001));
    }
    
    // 业务逻辑...
    
    return {};
}

// ========== 私有方法 ==========
void Example::_validate_input(const std::string &input) {
    if (input.empty()) {
        throw std::invalid_argument("Input cannot be empty");
    }
}

bool Example::_is_valid_state() const noexcept {
    return !d->id_.empty();
}

} // namespace youdidit
} // namespace xswl
```

#### 测试模板

```cpp
// tests/unit/test_example.cpp
#include <gtest/gtest.h>
#include <xswl/youdidit/core/example.hpp>

using namespace xswl::youdidit;

class ExampleTest : public ::testing::Test {
protected:
    void SetUp() override {
        example_ = std::make_unique<Example>("test-001");
    }
    
    std::unique_ptr<Example> example_;
};

TEST_F(ExampleTest, DefaultConstruction) {
    Example e;
    EXPECT_TRUE(e.id().empty());
}

TEST_F(ExampleTest, ConstructionWithId) {
    EXPECT_EQ(example_->id(), "test-001");
}

TEST_F(ExampleTest, SetAndGetName) {
    example_->set_name("Test Name");
    EXPECT_EQ(example_->name(), "Test Name");
}

TEST_F(ExampleTest, FluentApi) {
    example_->set_name("Name1").set_name("Name2");
    EXPECT_EQ(example_->name(), "Name2");
}

TEST_F(ExampleTest, DoSomething_Success) {
    auto result = example_->do_something("valid input");
    EXPECT_TRUE(result.has_value());
}

TEST_F(ExampleTest, DoSomething_InvalidState) {
    Example empty_example;
    auto result = empty_example.do_something("input");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code, 1001);
}
```

### 常见问题与解决方案

#### Q1：如何处理循环依赖？

```cpp
// 使用前向声明
// task.hpp
namespace xswl { namespace youdidit { class Claimer; } }

class Task {
    // 使用指针或 shared_ptr 引用 Claimer
    std::weak_ptr<Claimer> claimer_;
};
```

#### Q2：如何实现线程安全的状态转换？

```cpp
bool Task::_try_set_status_atomic(TaskStatus expected, TaskStatus desired) {
    return d->status_.compare_exchange_strong(
        expected, 
        desired,
        std::memory_order_acq_rel
    );
}
```

#### Q3：如何避免死锁？

遵循加锁顺序：
1. platform_mutex（最外层）
2. tasks_mutex
3. claimers_mutex
4. task_data_mutex（最内层）

```cpp
// 正确
std::unique_lock<std::shared_mutex> platform_lock(platform_mutex_);
std::shared_lock<std::shared_mutex> task_lock(task_mutex_);

// 错误（可能死锁）
std::shared_lock<std::shared_mutex> task_lock(task_mutex_);
std::unique_lock<std::shared_mutex> platform_lock(platform_mutex_);
```

#### Q4：如何正确使用信号槽？

```cpp
// 在 Impl 中定义信号
class Task::Impl {
    xswl::signal_t<TaskStatus> sig_status_changed_;
};

// 在公有接口中暴露
xswl::signal_t<TaskStatus> &Task::sig_status_changed() {
    return d->sig_status_changed_;
}

// 触发信号
void Task::set_status(TaskStatus status) {
    auto old = d->status_.exchange(status);
    if (old != status) {
        d->sig_status_changed_.emit(status);
    }
}
```

### 检查清单

每次提交代码前，请确认：

- [ ] 代码符合项目编码规范
- [ ] 所有公有方法有文档注释
- [ ] 私有方法使用下划线前缀
- [ ] Pimpl 指针命名为 `d`
- [ ] Getter 使用简洁命名
- [ ] 适当标注 `noexcept`
- [ ] 引用类型使用右对齐写法
- [ ] 已添加必要的单元测试
- [ ] 编译无警告
- [ ] 所有测试通过

---

## 版本历史

| 版本 | 日期 | 变更说明 |
|------|------|---------|
| 1.0.0 | 2026-01-27 | 初始版本 |

---

## 参考文档

- [API.md](../../API.md) - 完整 API 文档
- [README.md](../../README.md) - 项目概述
- [WEB_API.md](WEB_API.md) - Web API 文档
- [WEB_MONITORING.md](WEB_MONITORING.md) - Web 监控系统设计

# 开发者指南（Developer Guide）

本文档面向开发者，包含任务模型、API 摘要、示例与实现相关的设计说明。若你是普通使用者，请优先查看 `docs/usage.md` 与 `docs/api/API.md`。

---

## 错误与正确的设计示例

```cpp
// 不好的设计：申领者需要知道具体做什么
class DataProcessorClaimer : public Claimer {
    void process_task(const Task &task) override {
        // 申领者自己决定如何处理任务 - 这是错的！
        load_data(task.metadata["file"]);
        process_data();
        save_result();
    }
};
```

✅ **正确设计**：任务携带业务逻辑

```cpp
// 好的设计：任务本身定义业务逻辑
auto task = platform->task_builder()
    .handler([](Task &task, const auto &input) {
        // 发布者在这里定义任务要做什么
        load_data(input.at("file"));
        process_data();
        save_result();
        return TaskResult{};
    })
    .build();

// 申领者只负责执行
claimer->execute_task(task.id(), "/data/input.csv");
```

---

## 任务（Task）与 TaskHandler

任务（Task）是平台的核心实体，包含状态、进度与元数据；业务逻辑通过 `TaskHandler` 由发布者定义：

```cpp
using TaskHandler = std::function<TaskResult(
    Task &task,              // 任务对象本身（用于更新进度等）
    const std::string &input // 输入数据（格式由用户定义）
)>;
```

处理失败请返回包含错误信息的 `TaskResult` 或 `tl::expected` 风格错误。

### 核心属性（概览）

- id, title, description
- priority, category, tags
- publisher_id, claimer_id, required_role
- status, progress
- created_at, deadline, completed_at
- reward_points, reward_type

---

## 任务示例

```cpp
// 示例：文件路径作为输入
auto task1 = platform->task_builder()
    .title("数据处理任务")
    .priority(5)
    .handler([](Task &task, const std::string &input) -> TaskResult {
        task.set_progress(10);
        auto data = load_file(input);
        task.set_progress(50);
        auto result = process_data(data);
        task.set_progress(100);
        return TaskResult("处理完成");
    })
    .build();

// 使用 JSON 作为输入示例
auto task2 = platform->task_builder()
    .title("复杂分析任务")
    .handler([](Task &task, const std::string &input) -> TaskResult {
        auto config = nlohmann::json::parse(input);
        std::string file_path = config["file_path"];
        int batch_size = config.value("batch_size", 100);
        // ... 业务逻辑
        return TaskResult("分析完成");
    })
    .build();
```

---

## 任务状态（TaskStatus）

任务状态包括：Draft, Published, Claimed, Processing, Paused, Completed, Failed, Cancelled, Abandoned。详见 `docs/architecture/CONCEPTS.md` 中的状态图与语义说明。

---

## 角色（简介）

- 发布者（Publisher）：创建与发布任务
- 申领者（Claimer）：申领并执行任务
- 分派者（Dispatcher）：可指定任务处理者

角色的详细接口请参阅 `docs/api/API.md`。

---

## 申领者（Claimer）常用接口（摘要）

```cpp
class Claimer {
public:
    tl::expected<std::shared_ptr<Task>, Error> claim_task(const TaskId &task_id);
    tl::expected<std::shared_ptr<Task>, Error> claim_next_task();
    tl::expected<std::shared_ptr<Task>, Error> claim_matching_task();
    std::vector<std::shared_ptr<Task>> claim_tasks_to_capacity();

    tl::expected<void, Error> execute_task(const TaskId &task_id, const std::string &input = "");

    ClaimerState status() const noexcept; 
    // set_status 已移除：使用 set_paused()/set_offline()/set_max_concurrent() 等方法
    bool can_claim_more() const noexcept; 
};
```

---

## 生命周期（简要）

1. Draft -> Published
2. Published -> Claimed
3. Claimed -> Processing
4. Processing -> Completed / Failed / Abandoned

详见 `docs/architecture/CONCEPTS.md`。

---

## 开发者注意事项 & 参考

- 信号语义（同步/异步/是否在锁内）：`docs/architecture/SIGNAL_SEMANTICS.md`（Signal semantics）
- 异步执行与 Executor 设计讨论：`docs/maintenance/todo.md`（改进建议）
- API 参考：`docs/api/API.md`
- 使用示例：`docs/usage.md`

---

（本文件仅为开发者快速参考；如需贡献编码，请阅读 `CONTRIBUTION.md` 或 `docs/contributing.md`）

---

## 实现细节：现代 C++ 特性

项目在实现中使用了若干常见 C++ 特性以提高性能与可维护性：

| 特性 | 用途 |
|------|------|
| **智能指针** | 自动内存管理，避免手动 delete |
| **Lambda 表达式** | 灵活的回调和事件处理 |
| **Move 语义** | 高效的资源转移和避免拷贝 |
| **std::function** | 类型擦除的函数包装 |
| **Fluent API** | 链式调用，提升代码可读性 |
| **Builder 模式** | 复杂对象的灵活构建 |
| **Result/Optional** | 函数式的错误处理 |

> 提示：更详细的实现注意事项（如 noexcept、Pimpl 使用、线程约定）请参阅 `docs/architecture/CONCEPTS.md` 和代码注释。

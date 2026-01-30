# 使用示例（详解）

本文档收集了 README 中较长的示例与使用片段，便于用户在开发时复制、运行与修改。

## 基础示例（Fluent API）

```cpp
auto platform = std::make_shared<TaskPlatform>();

auto task = platform->task_builder()
    .title("Process Data")
    .priority(5)
    .handler([](Task &task, const std::string &input) -> TaskResult {
        task.set_progress(20);
        // 解析与处理
        task.set_progress(100);
        return TaskResult{true, "OK"};
    })
    .build();

auto claimer = std::make_shared<Claimer>("Worker-001");
claimer->claim_task(task->id());
claimer->execute_task(task->id(), "/data/input.csv");
```

## 高级示例（事件驱动）

```cpp
platform->sig_task_published().connect([](const std::shared_ptr<Task>& task) {
    std::cout << "New task published: " << task->title() << std::endl;
});

platform->sig_task_completed().connect([](const std::shared_ptr<Task>& task, const TaskResult& r) {
    std::cout << "Task completed: " << task->id() << std::endl;
});
```

## Web 监控示例

```cpp
auto platform = std::make_shared<TaskPlatform>();
auto dashboard = std::make_shared<WebDashboard>(platform.get());
dashboard->start_server(8080);
// 访问: http://localhost:8080
```

---

更多示例与用例请参阅：
- `examples/` 目录
- `docs/web/WEB_MONITORING.md`（监控相关示例）
- `docs/api/API.md`（接口使用与详细文档）

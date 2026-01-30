# 核心概念与设计（详解）

本文档从概念层面详细说明 `xswl-youdidit` 的设计原则、对象模型、任务生命周期与线程安全策略，适合需要理解内部语义与设计决策的读者。

## 目录
- 项目设计理念
- 任务（Task）与 TaskHandler
- 任务状态与生命周期
- 角色（发布者 / 申领者 / 分派者）
- 交互流程（序列图）
- 线程安全设计（原则与机制）
- 信号槽机制（事件系统）

---

## 项目设计理念

见 `README.md` 中的简要概述；这里重点说明一些设计决策的原因与约束：

- 单一职责：任务携带业务逻辑，申领者只负责执行。
- 可观测性：全链路日志、事件与时间回放支持调试与审计。
- 可扩展性：采用 Pimpl、接口化设计，便于未来扩展 Executor / TraceProvider 等。

---

## 任务（Task）与 TaskHandler

任务是系统的核心实体，包含状态、进度与元数据；业务逻辑通过 `TaskHandler` 由发布者定义：

```cpp
using TaskHandler = std::function<TaskResult(
    Task &task,
    const std::string &input
)>;
```

处理失败请返回包含错误信息的 `TaskResult` 或 `tl::expected` 风格错误。

---

## 任务状态与生命周期

任务遵循一组明确定义的状态（Draft, Published, Claimed, Processing, Paused, Completed, Failed, Cancelled, Abandoned），并在不同场景下做出有效的状态转换。时间点与修订由 Task 对象记录。

（详情请参阅本文件下方的状态转换图与说明）

---

## 角色

- 发布者（Publisher）：创建与发布任务
- 申领者（Claimer）：申领并执行任务
- 分派者（Dispatcher）：可指定任务处理者

每个角色拥有各自的权限与交互边界，参见 API 文档中的对应类说明。

---

## 交互流程（序列图）

以下 Mermaid 序列图展示了从任务发布到完成的完整交互流程：

```mermaid
sequenceDiagram
    actor Publisher as 发布者
    participant Platform as 任务平台
    participant Task as 任务对象
    actor Claimer as 申领者
    participant WebUI as Web监控

    %% 任务发布流程
    Publisher->>Platform: 创建任务
    Platform->>Task: 实例化 Task
    Publisher->>Task: 设置属性(title, priority, description)
    Publisher->>Platform: publish_task()
    Task->>Task: 状态: Draft → Published
    Task-->>Publisher: sig_published 信号
    Task-->>WebUI: 记录日志
    
    %% 任务申领流程
    Claimer->>Platform: 查询可申领任务
    Platform-->>Claimer: 返回任务列表
    Claimer->>Platform: claim_task(task_id)
    Platform->>Task: 检查任务状态
    Task->>Task: 状态: Published → Claimed
    Task-->>Claimer: sig_claimed 信号
    Task-->>Publisher: sig_claimed 信号
    Task-->>WebUI: 记录申领日志
    Platform-->>Claimer: 返回 TaskReference
    
    %% 任务处理流程
    Claimer->>Task: start_task()
    Task->>Task: 状态: Claimed → Processing
    Task-->>Claimer: sig_started 信号
    Task-->>Publisher: sig_started 信号
    Task-->>WebUI: 记录开始日志
    
    %% 进度更新流程
    loop 处理过程
        Claimer->>Task: update_progress(50)
        Task-->>Claimer: sig_progress_updated 信号
        Task-->>Publisher: sig_progress_updated 信号
        Task-->>WebUI: 实时更新进度
    end
    
    %% 任务完成流程
    Claimer->>Task: complete_task(result)
    Task->>Task: 状态: Processing → Completed
    Task-->>Claimer: sig_completed 信号
    Task-->>Publisher: sig_completed 信号
    Task-->>WebUI: 记录完成日志
    Platform->>Claimer: 更新统计信息(reputation_points++)
    
    %% 异常流程（可选）
    Note over Claimer,Task: 异常情况
    alt 任务失败
        Claimer->>Task: fail_task(reason)
        Task->>Task: 状态: Processing → Failed
        Task-->>Publisher: sig_failed 信号
        Task-->>WebUI: 记录失败日志
    else 任务放弃
        Claimer->>Task: abandon_task(reason)
        Task->>Task: 状态: Claimed/Processing → Abandoned
        Task-->>Publisher: sig_task_abandoned 信号
        Task-->>WebUI: 记录放弃日志
        Task->>Task: 状态: Abandoned → Published
    end
    
    %% Web监控回放
    WebUI->>WebUI: 时间回放功能
    Note over WebUI: 支持查看完整交互历史
```

---

## 线程安全设计

项目以线程安全为首要目标，采用：

- 细粒度锁（`std::shared_mutex` 与 `std::mutex`）
- 原子类型 (`std::atomic`) 用于频繁更新的基本属性（如进度、状态）
- 明确的加锁顺序以避免死锁（例如：`platform_mutex` -> `task_mutex` -> `data_mutex`）

并在文档中给出最佳实践、验证方法（ThreadSanitizer、压力测试、CI）与示例代码片段，便于开发者参考。

---

## 信号槽机制

事件驱动使用 `xswl-signals`，提供类型安全的信号/槽链接、优先级、一次性连接和自动生命周期管理。信号语义（同步/异步）与回调约定在 API 文档中有明确说明（推荐在回调中避免长时间阻塞或使用异步 offload）。

---

## 参考

- API 参考：`API.md`
- 使用示例：`docs/usage.md`
- 开发计划与阶段报告：`docs/architecture/DEVELOPMENT_PLAN.md`、`docs/architecture/PHASE1_REPORT.md` 等

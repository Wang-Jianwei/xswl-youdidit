# 并发执行保护修复

## 问题描述

当多个线程同时调用 `Claimer::run_task()` 执行同一个任务时，存在严重的并发安全问题：

1. **重复触发信号**：`sig_task_started` 会被多次触发
2. **重复执行**：任务的 `execute()` 方法会被多个线程同时调用
3. **统计错误**：`complete_task()` 或 `abandon_task()` 会被多次调用，导致：
   - `active_task_count` 被多次递减，可能变为负数
   - `total_completed` 或 `total_abandoned` 被重复计数
   - 任务可能被从 `claimed_tasks_` 中多次移除

## 修复方案

### 实现细节

在 `Claimer::Impl` 中添加了执行跟踪机制：

```cpp
// 正在执行的任务（用于防止并发执行同一任务）
std::set<TaskId> executing_tasks_;
```

在 `run_task()` 方法中添加了并发保护：

1. **执行前检查**：在执行任务前，先检查该任务是否已在执行中
2. **原子标记**：使用互斥锁保护，将任务ID添加到 `executing_tasks_` 集合
3. **RAII清理**：使用守卫对象确保函数退出时自动清理执行标记

### 代码示例

```cpp
TaskResult Claimer::run_task(std::shared_ptr<Task> task, const std::string &input) {
    // ... 前置检查 ...
    
    TaskId task_id = task->id();
    
    // 并发保护：确保同一任务不会被多个线程同时执行
    {
        std::lock_guard<std::mutex> lock(d->data_mutex_);
        // 检查任务是否已经在执行中
        if (d->executing_tasks_.find(task_id) != d->executing_tasks_.end()) {
            return Error("Task is already being executed by another thread", 
                        ErrorCode::TASK_STATUS_INVALID);
        }
        // 标记任务为执行中
        d->executing_tasks_.insert(task_id);
    }
    
    // RAII 守卫：确保函数退出时自动清理执行标记
    struct ExecutionGuard {
        Claimer::Impl* impl;
        TaskId task_id;
        ~ExecutionGuard() {
            std::lock_guard<std::mutex> lock(impl->data_mutex_);
            impl->executing_tasks_.erase(task_id);
        }
    };
    ExecutionGuard guard{d.get(), task_id};
    
    // 执行任务...
}
```

## 测试验证

创建了专门的并发测试 `test_concurrent_execution.cpp`：

- 5个线程同时执行同一个任务
- 预期结果：只有1个线程成功，其余4个线程收到并发保护错误
- 测试通过 ✓

## 技术亮点

1. **最小化锁持有时间**：只在标记和清理时持有锁，执行期间不持锁
2. **异常安全**：使用RAII确保即使发生异常也能正确清理
3. **清晰的错误信息**：明确告知用户任务已被其他线程执行
4. **无性能开销**：对于不同任务的并发执行无影响

## 影响范围

- ✅ 修复了 `Claimer::run_task()` 的并发安全问题
- ✅ 保持了对不同任务的并发执行能力
- ✅ 所有现有测试继续通过
- ✅ 无API变更，完全向后兼容

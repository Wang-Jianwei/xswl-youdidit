#include <xswl/youdidit/core/task.hpp>
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace xswl::youdidit;

// 测试辅助宏
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAILED: " << message << std::endl; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        if (test_func()) { \
            std::cout << "PASSED" << std::endl; \
        } else { \
            std::cout << "FAILED" << std::endl; \
            all_passed = false; \
        } \
    } while (0)

// ========== 测试用例 ==========

// 测试 1: 默认构造
bool test_default_construction() {
    Task task;
    TEST_ASSERT(!task.id().empty(), "Task should have an ID");
    TEST_ASSERT(task.title().empty(), "Title should be empty");
    TEST_ASSERT(task.status() == TaskStatus::Draft, "Initial status should be Draft");
    TEST_ASSERT(task.progress() == 0, "Initial progress should be 0");
    TEST_ASSERT(task.priority() == 0, "Initial priority should be 0");
    return true;
}

// 测试 2: 带 ID 构造
bool test_construction_with_id() {
    Task task("custom_task_id");
    TEST_ASSERT(task.id() == "custom_task_id", "Task ID should match");
    TEST_ASSERT(task.status() == TaskStatus::Draft, "Initial status should be Draft");
    return true;
}

// 测试 3: Setter 和 Getter
bool test_setters_and_getters() {
    Task task;
    
    task.set_title("Test Task")
        .set_description("This is a test task")
        .set_priority(5)
        .set_category("testing");
    
    TEST_ASSERT(task.title() == "Test Task", "Title should match");
    TEST_ASSERT(task.description() == "This is a test task", "Description should match");
    TEST_ASSERT(task.priority() == 5, "Priority should match");
    TEST_ASSERT(task.category() == "testing", "Category should match");
    
    return true;
}

// 测试 4: 标签操作
bool test_tag_operations() {
    Task task;
    
    task.add_tag("urgent")
        .add_tag("backend")
        .add_tag("database");
    
    auto tags = task.tags();
    TEST_ASSERT(tags.size() == 3, "Should have 3 tags");
    TEST_ASSERT(tags.find("urgent") != tags.end(), "Should contain 'urgent' tag");
    TEST_ASSERT(tags.find("backend") != tags.end(), "Should contain 'backend' tag");
    
    task.remove_tag("backend");
    tags = task.tags();
    TEST_ASSERT(tags.size() == 2, "Should have 2 tags after removal");
    TEST_ASSERT(tags.find("backend") == tags.end(), "Should not contain 'backend' tag");
    
    return true;
}

// 测试 5: 元数据操作
bool test_metadata_operations() {
    Task task;
    
    task.set_metadata("author", "Alice")
        .set_metadata("version", "1.0")
        .set_metadata("environment", "production");
    
    auto metadata = task.metadata();
    TEST_ASSERT(metadata.size() == 3, "Should have 3 metadata entries");
    TEST_ASSERT(metadata["author"] == "Alice", "Author metadata should match");
    TEST_ASSERT(metadata["version"] == "1.0", "Version metadata should match");
    
    task.remove_metadata("version");
    metadata = task.metadata();
    TEST_ASSERT(metadata.size() == 2, "Should have 2 metadata entries after removal");
    TEST_ASSERT(metadata.find("version") == metadata.end(), "Should not contain 'version' metadata");
    
    return true;
}

// 测试 6: 进度更新
bool test_progress_update() {
    Task task;
    
    bool signal_triggered = false;
    int received_progress = -1;
    
    task.on_progress_updated.connect([&](Task &t, int progress) {
        signal_triggered = true;
        received_progress = progress;
    });
    
    task.set_progress(50);
    TEST_ASSERT(task.progress() == 50, "Progress should be 50");
    TEST_ASSERT(signal_triggered, "Progress signal should be triggered");
    TEST_ASSERT(received_progress == 50, "Received progress should be 50");
    
    // 测试边界值
    task.set_progress(150);  // 应该被限制为 100
    TEST_ASSERT(task.progress() == 100, "Progress should be clamped to 100");
    
    task.set_progress(-10);  // 应该被限制为 0
    TEST_ASSERT(task.progress() == 0, "Progress should be clamped to 0");
    
    return true;
}

// 测试 7: 有效状态转换
bool test_valid_status_transitions() {
    Task task;
    
    // Draft → Published
    TEST_ASSERT(task.can_transition_to(TaskStatus::Published), "Can transition from Draft to Published");
    task.set_status(TaskStatus::Published);
    TEST_ASSERT(task.status() == TaskStatus::Published, "Status should be Published");
    
    // Published → Claimed
    TEST_ASSERT(task.can_transition_to(TaskStatus::Claimed), "Can transition from Published to Claimed");
    task.set_status(TaskStatus::Claimed);
    TEST_ASSERT(task.status() == TaskStatus::Claimed, "Status should be Claimed");
    
    // Claimed → Processing
    TEST_ASSERT(task.can_transition_to(TaskStatus::Processing), "Can transition from Claimed to Processing");
    task.set_status(TaskStatus::Processing);
    TEST_ASSERT(task.status() == TaskStatus::Processing, "Status should be Processing");
    
    // Processing → Completed
    TEST_ASSERT(task.can_transition_to(TaskStatus::Completed), "Can transition from Processing to Completed");
    task.set_status(TaskStatus::Completed);
    TEST_ASSERT(task.status() == TaskStatus::Completed, "Status should be Completed");
    
    return true;
}

// 测试 8: 无效状态转换
bool test_invalid_status_transitions() {
    Task task;
    
    // Draft → Claimed (无效)
    TEST_ASSERT(!task.can_transition_to(TaskStatus::Claimed), "Cannot transition from Draft to Claimed");
    task.set_status(TaskStatus::Claimed);
    TEST_ASSERT(task.status() == TaskStatus::Draft, "Status should remain Draft");
    
    // 从终态转换（无效）
    task.set_status(TaskStatus::Published);
    task.set_status(TaskStatus::Cancelled);
    TEST_ASSERT(!task.can_transition_to(TaskStatus::Published), "Cannot transition from Cancelled");
    task.set_status(TaskStatus::Published);
    TEST_ASSERT(task.status() == TaskStatus::Cancelled, "Status should remain Cancelled");
    
    return true;
}

// 测试 9: 状态变更信号
bool test_status_change_signals() {
    Task task;
    
    bool status_changed = false;
    TaskStatus old_status = TaskStatus::Draft;
    TaskStatus new_status = TaskStatus::Draft;
    
    task.on_status_changed.connect([&](Task &t, TaskStatus old_s, TaskStatus new_s) {
        status_changed = true;
        old_status = old_s;
        new_status = new_s;
    });
    
    task.set_status(TaskStatus::Published);
    TEST_ASSERT(status_changed, "Status change signal should be triggered");
    TEST_ASSERT(old_status == TaskStatus::Draft, "Old status should be Draft");
    TEST_ASSERT(new_status == TaskStatus::Published, "New status should be Published");
    
    return true;
}

// 测试 10: 申领者权限检查（黑名单）
bool test_claimer_blacklist() {
    Task task;
    
    task.add_to_blacklist("bad_claimer");
    
    TEST_ASSERT(!task.is_claimer_allowed("bad_claimer"), "Blacklisted claimer should not be allowed");
    TEST_ASSERT(task.is_claimer_allowed("good_claimer"), "Non-blacklisted claimer should be allowed");
    
    task.remove_from_blacklist("bad_claimer");
    TEST_ASSERT(task.is_claimer_allowed("bad_claimer"), "Removed claimer should be allowed");
    
    return true;
}

// 测试 11: 申领者权限检查（白名单）
bool test_claimer_whitelist() {
    Task task;
    
    task.add_to_whitelist("allowed_claimer");
    
    TEST_ASSERT(task.is_claimer_allowed("allowed_claimer"), "Whitelisted claimer should be allowed");
    TEST_ASSERT(!task.is_claimer_allowed("other_claimer"), "Non-whitelisted claimer should not be allowed");
    
    task.remove_from_whitelist("allowed_claimer");
    task.add_to_whitelist("another_claimer");
    
    TEST_ASSERT(!task.is_claimer_allowed("allowed_claimer"), "Removed claimer should not be allowed");
    TEST_ASSERT(task.is_claimer_allowed("another_claimer"), "New whitelisted claimer should be allowed");
    
    return true;
}

// 测试 12: 黑名单优先级高于白名单
bool test_blacklist_overrides_whitelist() {
    Task task;
    
    task.add_to_whitelist("claimer_1");
    task.add_to_blacklist("claimer_1");
    
    TEST_ASSERT(!task.is_claimer_allowed("claimer_1"), "Blacklist should override whitelist");
    
    return true;
}

// 测试 13: 任务执行（成功）
bool test_task_execution_success() {
    Task task;
    
    task.set_handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
        t.set_progress(50);
        return TaskResult{true, "Task completed successfully"};
    });
    
    task.set_status(TaskStatus::Published);
    task.set_status(TaskStatus::Claimed);
    
    auto result = task.execute("test_input");
    TEST_ASSERT(result.has_value(), "Execution should succeed");
    TEST_ASSERT(result.value().success, "Task result should indicate success");
    TEST_ASSERT(task.status() == TaskStatus::Completed, "Status should be Completed");
    TEST_ASSERT(task.progress() == 100, "Progress should be 100");
    
    return true;
}

// 测试 14: 任务执行（失败）
bool test_task_execution_failure() {
    Task task;
    
    task.set_handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
        return tl::make_unexpected(std::string("Execution failed"));
    });
    
    task.set_status(TaskStatus::Published);
    task.set_status(TaskStatus::Claimed);
    
    auto result = task.execute("test_input");
    TEST_ASSERT(!result.has_value(), "Execution should fail");
    TEST_ASSERT(task.status() == TaskStatus::Failed, "Status should be Failed");
    
    return true;
}

// 测试 15: 任务执行信号
bool test_task_execution_signals() {
    Task task;
    
    bool started_signal = false;
    bool completed_signal = false;
    TaskResult received_result{false, ""};
    
    task.on_started.connect([&](Task &t) {
        started_signal = true;
    });
    
    task.on_completed.connect([&](Task &t, const TaskResult &result) {
        completed_signal = true;
        received_result = result;
    });
    
    task.set_handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
        return TaskResult{true, "Completed"};
    });
    
    task.set_status(TaskStatus::Published);
    task.set_status(TaskStatus::Claimed);
    task.execute("input");
    
    TEST_ASSERT(started_signal, "Started signal should be triggered");
    TEST_ASSERT(completed_signal, "Completed signal should be triggered");
    TEST_ASSERT(received_result.success, "Received result should indicate success");
    
    return true;
}

// 测试 16: 时间戳
bool test_timestamps() {
    Task task;
    
    auto now = std::chrono::system_clock::now();
    
    TEST_ASSERT(task.created_at() <= now, "Created timestamp should be <= now");
    
    task.set_published_at(now);
    auto published = task.published_at();
    TEST_ASSERT(published.time_since_epoch().count() > 0, "Published timestamp should be set");
    
    return true;
}

// 测试 17: 协作式取消请求（标志与信号）
bool test_request_cancel_signal_and_flag() {
    Task task;

    bool signal_triggered = false;
    std::string received_reason;

    task.on_cancel_requested.connect([&](Task &t, const std::string &reason) {
        signal_triggered = true;
        received_reason = reason;
    });

    task.set_status(TaskStatus::Published);
    task.set_status(TaskStatus::Claimed);
    task.set_status(TaskStatus::Processing);

    auto res = task.request_cancel("publisher_cancel");
    TEST_ASSERT(res.has_value(), "request_cancel should succeed");
    TEST_ASSERT(task.is_cancel_requested(), "is_cancel_requested should be true");
    TEST_ASSERT(signal_triggered, "on_cancel_requested should be triggered");
    TEST_ASSERT(received_reason == "publisher_cancel", "Reason should match");

    auto md = task.metadata();
    TEST_ASSERT(md.find("cancel.reason") != md.end(), "Metadata should contain cancel.reason");
    TEST_ASSERT(md.find("cancel.requested_at") != md.end(), "Metadata should contain cancel.requested_at");
    TEST_ASSERT(md["cancel.reason"] == "publisher_cancel", "Metadata reason should match");
    TEST_ASSERT(!md["cancel.requested_at"].empty(), "Metadata requested_at should be non-empty");

    return true;
}

// 测试 18: 任务处理函数响应取消请求
bool test_handler_obeys_cancel_request() {
    Task task;

    task.set_handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
        for (int i = 0; i < 50; ++i) {
            if (t.is_cancel_requested()) {
                return tl::make_unexpected(std::string("cancelled"));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return TaskResult{true, "completed"};
    });

    task.set_status(TaskStatus::Published);
    task.set_status(TaskStatus::Claimed);

    std::atomic<bool> exec_done{false};
    std::atomic<bool> exec_failed{false};

    std::thread runner([&]() {
        auto result = task.execute("input");
        if (!result.has_value()) {
            exec_failed.store(true, std::memory_order_release);
        }
        exec_done.store(true, std::memory_order_release);
    });

    // 等待 handler 开始执行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    task.request_cancel("publisher_cancel_during_exec");

    // 等待执行结束
    runner.join();

    TEST_ASSERT(exec_done.load(std::memory_order_acquire), "Execution should finish");
    TEST_ASSERT(exec_failed.load(std::memory_order_acquire), "Execution should have aborted due to cancellation");

    return true;
}

// 测试 19: 移动语义
bool test_move_semantics() {
    Task task1("task_1");
    task1.set_title("Original Task");
    task1.set_priority(10);
    
    Task task2(std::move(task1));
    TEST_ASSERT(task2.id() == "task_1", "Moved task should have same ID");
    TEST_ASSERT(task2.title() == "Original Task", "Moved task should have same title");
    TEST_ASSERT(task2.priority() == 10, "Moved task should have same priority");
    
    Task task3;
    task3 = std::move(task2);
    TEST_ASSERT(task3.id() == "task_1", "Move-assigned task should have same ID");
    TEST_ASSERT(task3.title() == "Original Task", "Move-assigned task should have same title");
    
    return true;
}

// ========== 主函数 ==========
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "         Task Class Unit Tests         " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    bool all_passed = true;
    
    RUN_TEST(test_default_construction);
    RUN_TEST(test_construction_with_id);
    RUN_TEST(test_setters_and_getters);
    RUN_TEST(test_tag_operations);
    RUN_TEST(test_metadata_operations);
    RUN_TEST(test_progress_update);
    RUN_TEST(test_valid_status_transitions);
    RUN_TEST(test_invalid_status_transitions);
    RUN_TEST(test_status_change_signals);
    RUN_TEST(test_claimer_blacklist);
    RUN_TEST(test_claimer_whitelist);
    RUN_TEST(test_blacklist_overrides_whitelist);
    RUN_TEST(test_task_execution_success);
    RUN_TEST(test_task_execution_failure);
    RUN_TEST(test_task_execution_signals);
    RUN_TEST(test_timestamps);
    RUN_TEST(test_move_semantics);
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    if (all_passed) {
        std::cout << "  ✓ All tests PASSED" << std::endl;
    } else {
        std::cout << "  ✗ Some tests FAILED" << std::endl;
    }
    std::cout << "========================================" << std::endl;
    
    return all_passed ? 0 : 1;
}

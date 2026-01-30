#include <xswl/youdidit/core/claimer.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <xswl/youdidit/core/task_builder.hpp>
#include <cassert>
#include <iostream>
#include <memory>

using namespace xswl::youdidit;

// ========== 测试辅助函数 ==========
void assert_true(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << std::endl;
        std::exit(1);
    }
}

void assert_equal(int a, int b, const char* message) {
    if (a != b) {
        std::cerr << "Assertion failed: " << message 
                  << " (expected " << b << ", got " << a << ")" << std::endl;
        std::exit(1);
    }
}

void assert_equal(const std::string& a, const std::string& b, const char* message) {
    if (a != b) {
        std::cerr << "Assertion failed: " << message 
                  << " (expected '" << b << "', got '" << a << "')" << std::endl;
        std::exit(1);
    }
}

// ========== 测试用例 ==========

// 测试1: 构造函数
void test_constructor() {
    std::cout << "Test 1: Constructor... ";
    
    Claimer claimer("claimer-001", "Alice");
    
    assert_equal(claimer.id(), "claimer-001", "ID should match");
    assert_equal(claimer.name(), "Alice", "Name should match");
    assert_true(claimer.status() == ClaimerStatus::Idle, "Initial status should be Idle");
    assert_equal(claimer.max_concurrent_tasks(), 5, "Default max concurrent should be 5");
    assert_equal(claimer.active_task_count(), 0, "Initial active count should be 0");
    assert_equal(claimer.total_claimed(), 0, "Total claimed should be 0");
    assert_equal(claimer.total_completed(), 0, "Total completed should be 0");
    assert_equal(claimer.total_failed(), 0, "Total failed should be 0");
    assert_equal(claimer.total_abandoned(), 0, "Total abandoned should be 0");
    
    std::cout << "PASSED" << std::endl;
}

// 测试2: 设置名称
void test_set_name() {
    std::cout << "Test 2: Set name... ";
    
    Claimer claimer("claimer-002", "Bob");
    claimer.set_name("Robert");
    
    assert_equal(claimer.name(), "Robert", "Name should be updated");
    
    std::cout << "PASSED" << std::endl;
}

// 测试3: 设置状态和信号
void test_set_status_with_signal() {
    std::cout << "Test 3: Set status with signal... ";
    
    Claimer claimer("claimer-003", "Charlie");
    
    bool signal_emitted = false;
    ClaimerStatus old_status_captured = ClaimerStatus::Idle;
    ClaimerStatus new_status_captured = ClaimerStatus::Idle;
    
    claimer.sig_status_changed.connect([&](Claimer &, ClaimerStatus old_s, ClaimerStatus new_s) {
        signal_emitted = true;
        old_status_captured = old_s;
        new_status_captured = new_s;
    });
    
    // 测试 set_paused() - 状态从 Idle 变为 Paused
    claimer.set_paused(true);
    
    assert_true(signal_emitted, "Signal should be emitted");
    assert_true(old_status_captured == ClaimerStatus::Idle, "Old status should be Idle");
    assert_true(new_status_captured == ClaimerStatus::Paused, "New status should be Paused");
    assert_true(claimer.status() == ClaimerStatus::Paused, "Status should be Paused");
    
    // 重置信号标志
    signal_emitted = false;
    old_status_captured = ClaimerStatus::Idle;
    new_status_captured = ClaimerStatus::Idle;
    
    // 测试 set_offline() - 状态从 Paused 变为 Offline
    claimer.set_offline(true);
    
    assert_true(signal_emitted, "Signal should be emitted");
    assert_true(old_status_captured == ClaimerStatus::Paused, "Old status should be Paused");
    assert_true(new_status_captured == ClaimerStatus::Offline, "New status should be Offline");
    assert_true(claimer.status() == ClaimerStatus::Offline, "Status should be Offline");
    
    std::cout << "PASSED" << std::endl;
}

// 测试4: 设置最大并发任务数
void test_set_max_concurrent() {
    std::cout << "Test 4: Set max concurrent tasks... ";
    
    Claimer claimer("claimer-004", "David");
    claimer.set_max_concurrent(10);
    
    assert_equal(claimer.max_concurrent_tasks(), 10, "Max concurrent should be updated");
    
    std::cout << "PASSED" << std::endl;
}

// 测试5: 角色管理
void test_role_management() {
    std::cout << "Test 5: Role management... ";
    
    Claimer claimer("claimer-005", "Eve");
    
    claimer.add_role("developer")
           .add_role("reviewer")
           .add_role("tester");
    
    auto roles = claimer.roles();
    assert_equal(static_cast<int>(roles.size()), 3, "Should have 3 roles");
    assert_true(roles.find("developer") != roles.end(), "Should have developer role");
    assert_true(roles.find("reviewer") != roles.end(), "Should have reviewer role");
    assert_true(roles.find("tester") != roles.end(), "Should have tester role");
    
    claimer.remove_role("reviewer");
    roles = claimer.roles();
    assert_equal(static_cast<int>(roles.size()), 2, "Should have 2 roles after removal");
    assert_true(roles.find("reviewer") == roles.end(), "Reviewer role should be removed");
    
    std::cout << "PASSED" << std::endl;
}

// 测试6: 分类管理
void test_category_management() {
    std::cout << "Test 6: Category management... ";
    
    Claimer claimer("claimer-006", "Frank");
    
    claimer.add_category("backend")
           .add_category("frontend")
           .add_category("database");
    
    auto categories = claimer.categories();
    assert_equal(static_cast<int>(categories.size()), 3, "Should have 3 categories");
    assert_true(categories.find("backend") != categories.end(), "Should have backend category");
    
    claimer.remove_category("frontend");
    categories = claimer.categories();
    assert_equal(static_cast<int>(categories.size()), 2, "Should have 2 categories after removal");
    
    std::cout << "PASSED" << std::endl;
}

// 测试7: 申领单个任务
void test_claim_single_task() {
    std::cout << "Test 7: Claim single task... ";
    
    Claimer claimer("claimer-007", "Grace");
    claimer.add_category("development");
    
    // 创建一个任务
    TaskBuilder builder;
    auto task = builder.title("Test Task")
                      .description("Description")
                      .category("development")
                      .priority(80)
                      .handler([](Task&, const std::string &) {
                          return TaskResult("Success");
                      })
                      .build_and_publish();
    
    assert_true(task != nullptr, "Task should be created");
    
    bool claim_signal_emitted = false;
    claimer.sig_task_claimed.connect([&](Claimer &, std::shared_ptr<Task>) {
        claim_signal_emitted = true;
    });
    
    auto result = claimer.claim_task(task);
    
    assert_true(result.has_value(), "Claim should succeed");
    assert_true(claim_signal_emitted, "Claim signal should be emitted");
    assert_true(task->status() == TaskStatus::Claimed, "Task status should be Claimed");
    assert_equal(task->claimer_id(), "claimer-007", "Task claimer ID should match");
    assert_equal(claimer.active_task_count(), 1, "Active task count should be 1");
    assert_equal(claimer.total_claimed(), 1, "Total claimed should be 1");
    
    std::cout << "PASSED" << std::endl;
}

// 测试8: 检查最大并发限制
void test_max_concurrent_limit() {
    std::cout << "Test 8: Max concurrent limit... ";
    
    Claimer claimer("claimer-008", "Henry");
    claimer.set_max_concurrent(2);
    claimer.add_category("test");
    
    // 创建3个任务
    TaskBuilder builder;
    auto task1 = builder.title("Task 1")
                       .category("test")
                       .handler([](Task&, const std::string &) { return TaskResult(""); })
                       .build_and_publish();
    builder.reset();
    auto task2 = builder.title("Task 2")
                       .category("test")
                       .handler([](Task&, const std::string &) { return TaskResult(""); })
                       .build_and_publish();
    builder.reset();
    auto task3 = builder.title("Task 3")
                       .category("test")
                       .handler([](Task&, const std::string &) { return TaskResult(""); })
                       .build_and_publish();
    
    // 申领前两个任务应该成功
    auto result1 = claimer.claim_task(task1);
    auto result2 = claimer.claim_task(task2);
    
    assert_true(result1.has_value(), "First claim should succeed");
    assert_true(result2.has_value(), "Second claim should succeed");
    assert_true(claimer.can_claim_more() == false, "Should not be able to claim more");
    
    // 申领第三个任务应该失败
    auto result3 = claimer.claim_task(task3);
    assert_true(!result3.has_value(), "Third claim should fail");
    assert_true(result3.error().code == ErrorCode::CLAIMER_TOO_MANY_TASKS, 
                "Error should be too many tasks");
    
    std::cout << "PASSED" << std::endl;
}

// 测试9: 权限检查 - 黑名单
void test_permission_blacklist() {
    std::cout << "Test 9: Permission check - blacklist... ";
    
    Claimer claimer("claimer-009", "Ivy");
    claimer.add_category("test");
    
    TaskBuilder builder;
    auto task = builder.title("Restricted Task")
                      .category("test")
                      .blacklist("claimer-009")
                      .handler([](Task&, const std::string &) { return TaskResult(""); })
                      .build_and_publish();
    
    auto result = claimer.claim_task(task);
    
    assert_true(!result.has_value(), "Claim should fail for blacklisted claimer");
    assert_true(result.error().code == ErrorCode::CLAIMER_BLOCKED, 
                "Error should be claimer blocked");
    
    std::cout << "PASSED" << std::endl;
}

// 测试10: 权限检查 - 白名单
void test_permission_whitelist() {
    std::cout << "Test 10: Permission check - whitelist... ";
    
    Claimer claimer1("claimer-010-a", "Jack");
    Claimer claimer2("claimer-010-b", "Kate");
    
    claimer1.add_category("test");
    claimer2.add_category("test");
    
    TaskBuilder builder;
    auto task = builder.title("Exclusive Task")
                      .category("test")
                      .whitelist("claimer-010-a")
                      .handler([](Task&, const std::string &) { return TaskResult(""); })
                      .build_and_publish();
    
    auto result1 = claimer1.claim_task(task);
    assert_true(result1.has_value(), "Whitelisted claimer should succeed");
    
    // 先放弃任务以便第二个申领者尝试
    claimer1.abandon_task(task->id(), "Testing");
    task->set_status(TaskStatus::Published);
    
    auto result2 = claimer2.claim_task(task);
    assert_true(!result2.has_value(), "Non-whitelisted claimer should fail");
    
    std::cout << "PASSED" << std::endl;
}

// 测试11: 执行任务
void test_run_task() {
    std::cout << "Test 11: Run task... ";
    
    Claimer claimer("claimer-011", "Leo");
    claimer.add_category("compute");
    
    bool handler_called = false;
    TaskBuilder builder;
    auto task = builder.title("Compute Task")
                      .category("compute")
                      .handler([&](Task&, const std::string &input) {
                          handler_called = true;
                          return TaskResult("Computed: " + input);
                      })
                      .build_and_publish();
    
    claimer.claim_task(task);
    
    bool started_signal = false;
    bool completed_signal = false;
    
    claimer.sig_task_started.connect([&](Claimer &, std::shared_ptr<Task>) {
        started_signal = true;
    });
    
    claimer.sig_task_completed.connect([&](Claimer &, std::shared_ptr<Task>, const TaskResult &) {
        completed_signal = true;
    });
    
    auto result = claimer.run_task(task, "test-input");
    
    assert_true(result.ok(), "Execution should succeed");
    assert_true(handler_called, "Handler should be called");
    assert_true(started_signal, "Started signal should be emitted");
    assert_true(completed_signal, "Completed signal should be emitted");
    assert_equal(result.summary, "Computed: test-input", "Output should match");
    assert_equal(claimer.total_completed(), 1, "Total completed should be 1");
    
    std::cout << "PASSED" << std::endl;
}

// 测试12: 完成任务
void test_complete_task() {
    std::cout << "Test 12: Complete task... ";
    
    Claimer claimer("claimer-012", "Mia");
    claimer.add_category("test");
    
    TaskBuilder builder;
    auto task = builder.title("Task to Complete")
                      .category("test")
                      .handler([](Task&, const std::string &) { return TaskResult(""); })
                      .build_and_publish();
    
    claimer.claim_task(task);
    
    bool completed_signal = false;
    claimer.sig_task_completed.connect([&](Claimer &, std::shared_ptr<Task>, const TaskResult &) {
        completed_signal = true;
    });
    
    TaskResult result("Done");
    auto complete_result = claimer.complete_task(task->id(), result);
    
    std::cout << "Task status: " << static_cast<int>(task->status()) << " (Expected: " << static_cast<int>(TaskStatus::Completed) << ")" << std::endl;
    
    assert_true(complete_result.has_value(), "Complete should succeed");
    assert_true(completed_signal, "Completed signal should be emitted");
    assert_true(task->status() == TaskStatus::Completed, "Task should be completed");
    assert_equal(task->progress(), 100, "Progress should be 100");
    assert_equal(claimer.total_completed(), 1, "Total completed should be 1");
    
    std::cout << "PASSED" << std::endl;
}

// 测试13: 放弃任务
void test_abandon_task() {
    std::cout << "Test 13: Abandon task... ";
    
    Claimer claimer("claimer-013", "Nina");
    claimer.add_category("test");
    
    TaskBuilder builder;
    auto task = builder.title("Task to Abandon")
                      .category("test")
                      .handler([](Task&, const std::string &) { return TaskResult(""); })
                      .build_and_publish();
    
    claimer.claim_task(task);
    assert_equal(claimer.active_task_count(), 1, "Active count should be 1");
    
    bool abandoned_signal = false;
    claimer.sig_task_abandoned.connect([&](Claimer &, std::shared_ptr<Task>, const std::string &) {
        abandoned_signal = true;
    });
    
    auto result = claimer.abandon_task(task->id(), "Too difficult");
    
    assert_true(result.has_value(), "Abandon should succeed");
    assert_true(abandoned_signal, "Abandoned signal should be emitted");
    assert_true(task->status() == TaskStatus::Abandoned, "Task should be abandoned");
    assert_equal(claimer.total_abandoned(), 1, "Total abandoned should be 1");
    assert_equal(claimer.active_task_count(), 0, "Active count should be 0");
    
    std::cout << "PASSED" << std::endl;
}

// 测试14: 暂停和恢复任务
void test_pause_resume_task() {
    std::cout << "Test 14: Pause and resume task... ";
    
    Claimer claimer("claimer-014", "Oscar");
    claimer.add_category("test");
    
    TaskBuilder builder;
    auto task = builder.title("Task to Pause")
                      .category("test")
                      .handler([](Task&, const std::string &) { return TaskResult(""); })
                      .build_and_publish();
    
    claimer.claim_task(task);
    task->set_status(TaskStatus::Processing);
    
    auto pause_result = claimer.pause_task(task->id());
    assert_true(pause_result.has_value(), "Pause should succeed");
    assert_true(task->status() == TaskStatus::Paused, "Task should be paused");
    
    auto resume_result = claimer.resume_task(task->id());
    assert_true(resume_result.has_value(), "Resume should succeed");
    assert_true(task->status() == TaskStatus::Processing, "Task should be processing");
    
    std::cout << "PASSED" << std::endl;
}

// 测试15: 任务查询方法
void test_task_query_methods() {
    std::cout << "Test 15: Task query methods... ";
    
    Claimer claimer("claimer-015", "Paul");
    claimer.add_category("test");
    
    TaskBuilder builder;
    auto task1 = builder.title("Task 1")
                       .category("test")
                       .handler([](Task&, const std::string &) { return TaskResult(""); })
                       .build_and_publish();
    builder.reset();
    auto task2 = builder.title("Task 2")
                       .category("test")
                       .handler([](Task&, const std::string &) { return TaskResult(""); })
                       .build_and_publish();
    
    claimer.claim_task(task1);
    claimer.claim_task(task2);
    
    // 测试 has_task
    assert_true(claimer.has_task(task1->id()), "Should have task 1");
    assert_true(claimer.has_task(task2->id()), "Should have task 2");
    assert_true(!claimer.has_task("non-existent-id"), "Should not have non-existent task");
    
    // 测试 get_task
    auto task_opt = claimer.get_task(task1->id());
    assert_true(task_opt.has_value(), "Should get task 1");
    assert_equal(task_opt.value()->id(), task1->id(), "Task ID should match");
    
    auto missing_task = claimer.get_task("non-existent-id");
    assert_true(!missing_task.has_value(), "Should not get non-existent task");
    
    // 测试 claimed_tasks
    auto claimed = claimer.claimed_tasks();
    assert_equal(static_cast<int>(claimed.size()), 2, "Should have 2 claimed tasks");
    
    // 测试 active_tasks
    auto active = claimer.active_tasks();
    assert_equal(static_cast<int>(active.size()), 2, "Should have 2 active tasks");
    
    std::cout << "PASSED" << std::endl;
}

// 测试16: 匹配分数计算
void test_match_score_calculation() {
    std::cout << "Test 16: Match score calculation... ";
    
    Claimer claimer("claimer-016", "Quinn");
    claimer.add_category("backend");
    claimer.add_category("database");
    
    // 完全匹配的任务
    TaskBuilder builder;
    auto task1 = builder.title("Backend Task")
                       .category("backend")
                       .priority(100)
                       .add_tag("backend")
                       .handler([](Task&, const std::string &) { return TaskResult(""); })
                       .build();
    
    int score1 = claimer.calculate_match_score(task1);
    assert_true(score1 > 50, "Score for matching category should be > 50");
    
    // 不匹配的任务
    builder.reset();
    auto task2 = builder.title("Frontend Task")
                       .category("frontend")
                       .priority(0)
                       .handler([](Task&, const std::string &) { return TaskResult(""); })
                       .build();
    
    int score2 = claimer.calculate_match_score(task2);
    assert_true(score2 < score1, "Score for non-matching category should be lower");
    
    // 空任务
    int score3 = claimer.calculate_match_score(nullptr);
    assert_equal(score3, 0, "Score for null task should be 0");
    
    std::cout << "PASSED" << std::endl;
}

// 测试17: 移动语义
void test_move_semantics() {
    std::cout << "Test 17: Move semantics... ";
    
    Claimer claimer1("claimer-017", "Rachel");
    claimer1.add_role("developer");
    claimer1.set_max_concurrent(10);
    
    Claimer claimer2(std::move(claimer1));
    
    assert_equal(claimer2.id(), "claimer-017", "Moved claimer should have correct ID");
    assert_equal(claimer2.name(), "Rachel", "Moved claimer should have correct name");
    assert_equal(claimer2.max_concurrent_tasks(), 10, "Moved claimer should preserve settings");
    
    std::cout << "PASSED" << std::endl;
}

// ========== 主函数 ==========
int main() {
    std::cout << "Running Claimer unit tests..." << std::endl;
    std::cout << "================================" << std::endl;
    
    test_constructor();
    test_set_name();
    test_set_status_with_signal();
    test_set_max_concurrent();
    test_role_management();
    test_category_management();
    test_claim_single_task();
    test_max_concurrent_limit();
    test_permission_blacklist();
    test_permission_whitelist();
    test_run_task();
    test_complete_task();
    test_abandon_task();
    test_pause_resume_task();
    test_task_query_methods();
    test_match_score_calculation();
    test_move_semantics();
    
    std::cout << "================================" << std::endl;
    std::cout << "All tests passed!" << std::endl;
    
    return 0;
}

#include <xswl/youdidit/core/task_platform.hpp>
#include <cassert>
#include <iostream>

using namespace xswl::youdidit;

// 简单断言工具
void assert_true(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << std::endl;
        std::exit(1);
    }
}

void assert_equal(int lhs, int rhs, const char* message) {
    if (lhs != rhs) {
        std::cerr << "Assertion failed: " << message
                  << " (expected " << rhs << ", got " << lhs << ")" << std::endl;
        std::exit(1);
    }
}

void assert_equal(const std::string& lhs, const std::string& rhs, const char* message) {
    if (lhs != rhs) {
        std::cerr << "Assertion failed: " << message
                  << " (expected '" << rhs << "', got '" << lhs << "')" << std::endl;
        std::exit(1);
    }
}

// ========== 测试用例 ==========
void test_constructor() {
    std::cout << "Test 1: Constructor... ";
    TaskPlatform platform;
    assert_true(!platform.platform_id().empty(), "Platform id should not be empty");
    assert_true(platform.name().empty(), "Default name should be empty");
    assert_equal(static_cast<int>(platform.task_count()), 0, "Initial task count should be 0");
    assert_equal(static_cast<int>(platform.claimer_count()), 0, "Initial claimer count should be 0");
    std::cout << "PASSED" << std::endl;
}

void test_publish_and_get_task() {
    std::cout << "Test 2: Publish and get task... ";
    TaskPlatform platform;
    auto task = platform.task_builder()
                    .title("Publish Task")
                    .priority(60)
                    .handler([](Task&, const std::string&) { return TaskResult("ok"); })
                    .build_and_publish();
    platform.publish_task(task);

    assert_true(platform.has_task(task->id()), "Platform should contain task");
    auto fetched = platform.get_task(task->id());
    assert_true(fetched != nullptr, "Fetched task should not be null");
    assert_true(fetched->status() == TaskStatus::Published || fetched->status() == TaskStatus::Claimed,
                "Task should be published or claimed");
    std::cout << "PASSED" << std::endl;
}

void test_register_and_claim_by_id() {
    std::cout << "Test 3: Register claimer and claim by id... ";
    auto platform = std::make_shared<TaskPlatform>();
    auto claimer = std::make_shared<Claimer>("claimer-01", "Alice");
    claimer->add_category("dev");
    platform->register_claimer(claimer);

    auto task = platform->task_builder()
                    .title("Dev Task")
                    .category("dev")
                    .priority(50)
                    .handler([](Task&, const std::string&) { return TaskResult("done"); })
                    .build_and_publish();
    platform->publish_task(task);

    auto result = claimer->claim_task(task->id());
    assert_true(result.has_value(), "Claim should succeed");
    assert_true(task->status() == TaskStatus::Claimed, "Task should be claimed");
    assert_equal(task->claimer_id(), "claimer-01", "Claimer id should be set on task");
    assert_equal(claimer->claimed_task_count(), 1, "Claimer claimed tasks should be 1");
    std::cout << "PASSED" << std::endl;
}

void test_claim_next_task_priority() {
    std::cout << "Test 4: Claim next task by priority... ";
    auto platform = std::make_shared<TaskPlatform>();
    auto claimer = std::make_shared<Claimer>("claimer-02", "Bob");
    claimer->add_category("general");
    platform->register_claimer(claimer);

    auto low = platform->task_builder()
                   .title("Low Priority")
                   .priority(10)
                   .category("general")
                   .handler([](Task&, const std::string&) { return TaskResult("low"); })
                   .build_and_publish();
    auto high = platform->task_builder()
                    .title("High Priority")
                    .priority(90)
                    .category("general")
                    .handler([](Task&, const std::string&) { return TaskResult("high"); })
                    .build_and_publish();
    platform->publish_task(low);
    platform->publish_task(high);

    auto result = claimer->claim_next_task();
    assert_true(result.has_value(), "Should claim a task");
    assert_true(result.value()->id() == high->id(), "Should claim highest priority task");
    assert_true(high->status() == TaskStatus::Claimed, "High task should be claimed");
    assert_true(low->status() == TaskStatus::Published, "Low task should remain published");
    std::cout << "PASSED" << std::endl;
}

void test_claim_matching_task() {
    std::cout << "Test 5: Claim matching task... ";
    auto platform = std::make_shared<TaskPlatform>();
    auto claimer = std::make_shared<Claimer>("claimer-03", "Carol");
    claimer->add_category("backend");
    platform->register_claimer(claimer);

    auto backend = platform->task_builder()
                        .title("Backend Task")
                        .priority(40)
                        .category("backend")
                        .handler([](Task&, const std::string&) { return TaskResult("backend"); })
                        .build_and_publish();
    auto frontend = platform->task_builder()
                         .title("Frontend Task")
                         .priority(80)
                         .category("frontend")
                         .handler([](Task&, const std::string&) { return TaskResult("frontend"); })
                         .build_and_publish();
    platform->publish_task(backend);
    platform->publish_task(frontend);

    auto result = claimer->claim_matching_task();
    assert_true(result.has_value(), "Should claim a task");
    assert_true(result.value()->id() == backend->id(), "Should prefer matching category");
    std::cout << "PASSED" << std::endl;
}

void test_claim_tasks_to_capacity() {
    std::cout << "Test 6: Claim tasks to capacity... ";
    auto platform = std::make_shared<TaskPlatform>();
    auto claimer = std::make_shared<Claimer>("claimer-04", "Dave");
    claimer->add_category("ops");
    claimer->set_max_concurrent(2);
    platform->register_claimer(claimer);

    for (int i = 0; i < 3; ++i) {
        auto task = platform->task_builder()
                        .title("Ops Task " + std::to_string(i))
                        .priority(30 + i)
                        .category("ops")
                        .handler([](Task&, const std::string&) { return TaskResult("ops"); })
                        .build_and_publish();
        platform->publish_task(task);
    }

    auto tasks = claimer->claim_tasks_to_capacity();
    assert_equal(static_cast<int>(tasks.size()), 2, "Should claim up to capacity");
    assert_equal(claimer->claimed_task_count(), 2, "Claimed task count should be 2");
    std::cout << "PASSED" << std::endl;
}

void test_task_filters() {
    std::cout << "Test 7: Task filters... ";
    TaskPlatform platform;
    auto task1 = platform.task_builder()
                     .title("CatA")
                     .category("A")
                     .priority(10)
                     .handler([](Task&, const std::string&) { return TaskResult("a"); })
                     .build_and_publish();
    auto task2 = platform.task_builder()
                     .title("CatB")
                     .category("B")
                     .priority(50)
                     .handler([](Task&, const std::string&) { return TaskResult("b"); })
                     .build_and_publish();
    platform.publish_task(task1);
    platform.publish_task(task2);

    TaskPlatform::TaskFilter filter;
    filter.category = std::string("B");
    auto tasks = platform.get_tasks(filter);
    assert_equal(static_cast<int>(tasks.size()), 1, "Should filter by category");
    assert_equal(tasks[0]->title(), std::string("CatB"), "Filtered task should be CatB");
    std::cout << "PASSED" << std::endl;
}

void test_statistics() {
    std::cout << "Test 8: Statistics... ";
    auto platform = std::make_shared<TaskPlatform>();
    auto claimer = std::make_shared<Claimer>("claimer-05", "Eva");
    claimer->add_category("data");
    platform->register_claimer(claimer);

    auto task = platform->task_builder()
                    .title("Data Task")
                    .category("data")
                    .priority(70)
                    .handler([](Task&, const std::string&) { return TaskResult("data"); })
                    .build_and_publish();
    platform->publish_task(task);

    // Claim and complete
    claimer->claim_task(task->id());
    claimer->complete_task(task->id(), TaskResult("done"));

    auto stats = platform->get_statistics();
    assert_equal(static_cast<int>(stats.total_tasks), 1, "Total tasks should be 1");
    assert_equal(static_cast<int>(stats.completed_tasks), 1, "Completed tasks should be 1");
    assert_equal(static_cast<int>(stats.total_claimers), 1, "Total claimers should be 1");
    std::cout << "PASSED" << std::endl;
}

void test_cancel_published_task() {
    std::cout << "Test 9: Cancel published task... ";
    TaskPlatform platform;
    auto task = platform.task_builder()
                    .title("Cancelable Task")
                    .priority(50)
                    .handler([](Task&, const std::string&) { return TaskResult("ok"); })
                    .build_and_publish();
    platform.publish_task(task);

    bool cancelled_signal = false;
    platform.sig_task_cancelled.connect([&](const std::shared_ptr<Task> &) {
        cancelled_signal = true;
    });

    bool res = platform.cancel_task(task->id());
    assert_true(res, "cancel_task should return true");
    assert_true(cancelled_signal, "sig_task_cancelled should be emitted");
    assert_true(task->status() == TaskStatus::Cancelled, "Task should be Cancelled");
    std::cout << "PASSED" << std::endl;
}

void test_cancel_processing_task_emits_request_signal() {
    std::cout << "Test 10: Cancel processing task emits request signal... ";
    TaskPlatform platform;
    auto task = platform.task_builder()
                    .title("Processing Task")
                    .priority(60)
                    .handler([](Task&, const std::string&) { return TaskResult("ok"); })
                    .build_and_publish();
    platform.publish_task(task);

    // Simulate it being in processing state
    task->set_status(TaskStatus::Claimed);
    task->set_status(TaskStatus::Processing);

    bool request_signal = false;
    std::string received_reason;
    platform.sig_task_cancel_requested.connect([&](const std::shared_ptr<Task> &, const std::string &reason) {
        request_signal = true;
        received_reason = reason;
    });

    bool res = platform.cancel_task(task->id());
    assert_true(res, "cancel_task should return true");
    assert_true(request_signal, "sig_task_cancel_requested should be emitted");
    assert_true(task->is_cancel_requested(), "Task should have cancel requested flag set");
    assert_true(received_reason == "Cancelled by publisher", "Reason should match default");

    auto md = task->metadata();
    assert_true(md.find("cancel.reason") != md.end(), "Metadata should contain cancel.reason");
    assert_true(md.find("cancel.requested_at") != md.end(), "Metadata should contain cancel.requested_at");

    std::cout << "PASSED" << std::endl;
}

void test_clear_completed_tasks_behaviour() {
    std::cout << "Test 11: Clear completed tasks behaviour... ";
    TaskPlatform platform;

    auto t1 = platform.task_builder()
                  .title("C1")
                  .handler([](Task&, const std::string&){ return TaskResult("ok"); })
                  .build_and_publish();
    auto t2 = platform.task_builder()
                  .title("C2")
                  .handler([](Task&, const std::string&){ return TaskResult("ok"); })
                  .build_and_publish();
    platform.publish_task(t1);
    platform.publish_task(t2);

    // mark both as completed via valid transitions
    t1->set_status(TaskStatus::Claimed);
    t1->set_status(TaskStatus::Processing);
    t1->set_status(TaskStatus::Completed);

    t2->set_status(TaskStatus::Claimed);
    t2->set_status(TaskStatus::Processing);
    t2->set_status(TaskStatus::Completed);

    // t1 auto_cleanup = true, t2 default false
    t1->set_auto_cleanup(true);

    bool deleted_signal_t1 = false;
    bool deleted_signal_t2 = false;
    platform.sig_task_deleted.connect([&](const std::shared_ptr<Task> &t) {
        if (t->id() == t1->id()) deleted_signal_t1 = true;
        if (t->id() == t2->id()) deleted_signal_t2 = true;
    });

    // Only delete auto cleanup
    platform.clear_completed_tasks(true);
    assert_true(deleted_signal_t1, "t1 should be deleted when auto_cleanup is true");
    assert_true(platform.has_task(t2->id()), "t2 should remain when auto_cleanup is false");

    // Now delete regardless of auto_cleanup
    platform.clear_completed_tasks(false);
    assert_true(!platform.has_task(t2->id()), "t2 should be deleted when only_auto_clean=false");

    std::cout << "PASSED" << std::endl;
}

// ========== 主函数 ==========
int main() {
    std::cout << "Running TaskPlatform unit tests..." << std::endl;
    std::cout << "================================" << std::endl;

    test_constructor();
    test_publish_and_get_task();
    test_register_and_claim_by_id();
    test_claim_next_task_priority();
    test_claim_matching_task();
    test_claim_tasks_to_capacity();
    test_task_filters();
    test_statistics();
    test_cancel_published_task();
    test_cancel_processing_task_emits_request_signal();
    test_clear_completed_tasks_behaviour();

    std::cout << "================================" << std::endl;
    std::cout << "All tests passed!" << std::endl;
    return 0;
}

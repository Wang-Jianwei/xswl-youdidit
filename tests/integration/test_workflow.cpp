#include <xswl/youdidit/youdidit.hpp>
#include <iostream>
#include <vector>

using namespace xswl::youdidit;

namespace {
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " << msg << std::endl; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "Running " << #fn << "... "; \
        if (fn()) { \
            std::cout << "PASSED" << std::endl; \
        } else { \
            std::cout << "FAILED" << std::endl; \
            all_passed = false; \
        } \
    } while (0)

std::shared_ptr<Task> publish_sample(TaskPlatform &platform,
                                     const std::string &title,
                                     int priority,
                                     const std::string &category) {
    auto builder = platform.task_builder();
    builder.title(title)
           .description("integration task")
           .priority(priority)
           .category(category)
           .handler([](Task & /*task*/, const std::string &input) -> tl::expected<TaskResult, std::string> {
               TaskResult result(true, "processed" + input);
               result.output = input;
               return result;
           });
    auto task = builder.build();
    platform.publish_task(task);
    return task;
}

bool test_task_lifecycle() {
    TaskPlatform platform;
    auto claimer = std::make_shared<Claimer>("claimer-1", "Alice");
    claimer->add_category("data");
    platform.register_claimer(claimer);

    auto task = publish_sample(platform, "Integration Task", 80, "data");
    TEST_ASSERT(task->status() == TaskStatus::Published, "Task should be published");

    auto claim_result = claimer->claim_next_task();
    TEST_ASSERT(claim_result.has_value(), "Claimer should claim next task");
    auto claimed = claim_result.value();
    TEST_ASSERT(claimed->status() == TaskStatus::Claimed, "Status should be Claimed after claim");
    TEST_ASSERT(claimed->claimer_id() == claimer->id(), "Claimer id should be recorded");

    // 使用 run_task：执行并自动完成记账
    auto exec_result = claimer->run_task(claimed->id(), "::payload::");
    TEST_ASSERT(exec_result.has_value(), "Execution should succeed");
    TEST_ASSERT(claimed->status() == TaskStatus::Completed, "Task should be completed after run_task");
    TEST_ASSERT(claimer->total_completed() == 1, "Claimer completed counter should increment");

    auto stats = platform.get_statistics();
    TEST_ASSERT(stats.completed_tasks == 1, "Platform should record one completed task");
    TEST_ASSERT(stats.total_tasks == 1, "Platform total task count should be one");

    return true;
}

bool test_capacity_and_matching() {
    TaskPlatform platform;
    auto claimer = std::make_shared<Claimer>("claimer-2", "Bob");
    claimer->add_category("analytics");
    claimer->add_category("ops");
    claimer->set_max_concurrent(2);
    platform.register_claimer(claimer);

    auto t1 = publish_sample(platform, "Analytics", 60, "analytics");
    auto t2 = publish_sample(platform, "Operations", 40, "ops");
    TEST_ASSERT(t1 && t2, "Tasks should be created");

    auto claimed = claimer->claim_tasks_to_capacity();
    TEST_ASSERT(claimed.size() == 2, "Claimer should pick two tasks up to capacity");

    std::vector<std::string> seen_ids;
    for (const auto &task : claimed) {
        TEST_ASSERT(task->status() == TaskStatus::Claimed, "Claimed tasks remain in Claimed status");
        seen_ids.push_back(task->id());
    }

    TEST_ASSERT(claimer->active_task_count() == 2, "Active task count should reflect capacity");

    return true;
}

} // namespace

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "        Integration Workflow Tests      " << std::endl;
    std::cout << "========================================" << std::endl;

    bool all_passed = true;
    RUN_TEST(test_task_lifecycle);
    RUN_TEST(test_capacity_and_matching);

    std::cout << "========================================" << std::endl;
    if (all_passed) {
        std::cout << "  ✓ All integration workflow tests PASSED" << std::endl;
    } else {
        std::cout << "  ✗ Some integration workflow tests FAILED" << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return all_passed ? 0 : 1;
}

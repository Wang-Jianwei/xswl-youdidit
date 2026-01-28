#include <xswl/youdidit/core/task_builder.hpp>
#include <iostream>
#include <cassert>

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
    TaskBuilder builder;
    TEST_ASSERT(!builder.is_valid(), "Builder should not be valid without required fields");
    return true;
}

// 测试 2: Fluent API
bool test_fluent_api() {
    TaskBuilder builder;
    
    auto &result = builder.title("Test Task")
                          .description("Test Description")
                          .priority(50);
    
    // Fluent API 应该返回自身引用
    TEST_ASSERT(&result == &builder, "Fluent API should return self reference");
    
    return true;
}

// 测试 3: 基本验证 - 缺少标题
bool test_validation_missing_title() {
    TaskBuilder builder;
    
    builder.description("Description")
           .priority(50)
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           });
    
    TEST_ASSERT(!builder.is_valid(), "Should be invalid without title");
    
    auto errors = builder.validation_errors();
    TEST_ASSERT(!errors.empty(), "Should have validation errors");
    TEST_ASSERT(errors[0].find("title") != std::string::npos, "Error should mention title");
    
    return true;
}

// 测试 4: 基本验证 - 缺少处理函数
bool test_validation_missing_handler() {
    TaskBuilder builder;
    
    builder.title("Test Task")
           .description("Description")
           .priority(50);
    
    TEST_ASSERT(!builder.is_valid(), "Should be invalid without handler");
    
    auto errors = builder.validation_errors();
    bool found_handler_error = false;
    for (const auto &err : errors) {
        if (err.find("handler") != std::string::npos) {
            found_handler_error = true;
            break;
        }
    }
    TEST_ASSERT(found_handler_error, "Should have handler error");
    
    return true;
}

// 测试 5: 优先级范围验证
bool test_validation_priority_range() {
    TaskBuilder builder;
    
    builder.title("Test Task")
           .priority(-10)  // 无效优先级
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           });
    
    auto errors = builder.validation_errors();
    bool found_priority_error = false;
    for (const auto &err : errors) {
        if (err.find("priority") != std::string::npos) {
            found_priority_error = true;
            break;
        }
    }
    TEST_ASSERT(found_priority_error, "Should have priority range error");
    
    // 测试超出上限
    builder.reset()
           .title("Test Task")
           .priority(150)  // 无效优先级
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           });
    
    errors = builder.validation_errors();
    found_priority_error = false;
    for (const auto &err : errors) {
        if (err.find("priority") != std::string::npos) {
            found_priority_error = true;
            break;
        }
    }
    TEST_ASSERT(found_priority_error, "Should have priority range error for upper bound");
    
    return true;
}

// 测试 6: 标题长度验证
bool test_validation_title_length() {
    TaskBuilder builder;
    
    std::string long_title(250, 'a');  // 250个字符
    
    builder.title(long_title)
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           });
    
    auto errors = builder.validation_errors();
    bool found_title_error = false;
    for (const auto &err : errors) {
        if (err.find("title") != std::string::npos && err.find("long") != std::string::npos) {
            found_title_error = true;
            break;
        }
    }
    TEST_ASSERT(found_title_error, "Should have title length error");
    
    return true;
}

// 测试 7: 成功构建任务
bool test_build_success() {
    TaskBuilder builder;
    
    builder.title("Valid Task")
           .description("Valid Description")
           .priority(50)
           .category("testing")
           .add_tag("unit-test")
           .add_tag("automation")
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Completed"};
           })
           .metadata("author", "test")
           .metadata("version", "1.0");
    
    TEST_ASSERT(builder.is_valid(), "Builder should be valid");
    
    auto task = builder.build();
    TEST_ASSERT(task != nullptr, "Build should return a valid task");
    TEST_ASSERT(task->title() == "Valid Task", "Task title should match");
    TEST_ASSERT(task->description() == "Valid Description", "Task description should match");
    TEST_ASSERT(task->priority() == 50, "Task priority should match");
    TEST_ASSERT(task->category() == "testing", "Task category should match");
    TEST_ASSERT(task->status() == TaskStatus::Draft, "Initial status should be Draft");
    
    // 验证标签
    auto tags = task->tags();
    TEST_ASSERT(tags.size() == 2, "Should have 2 tags");
    TEST_ASSERT(tags.find("unit-test") != tags.end(), "Should have unit-test tag");
    TEST_ASSERT(tags.find("automation") != tags.end(), "Should have automation tag");
    
    // 验证元数据
    auto metadata = task->metadata();
    TEST_ASSERT(metadata.size() == 2, "Should have 2 metadata entries");
    TEST_ASSERT(metadata["author"] == "test", "Author metadata should match");
    TEST_ASSERT(metadata["version"] == "1.0", "Version metadata should match");
    
    return true;
}

// 测试 8: 构建失败（无效配置）
bool test_build_failure() {
    TaskBuilder builder;
    
    builder.title("Task without handler");
    // 缺少处理函数
    
    auto task = builder.build();
    TEST_ASSERT(task == nullptr, "Build should return nullptr for invalid configuration");
    
    return true;
}

// 测试 9: 白名单和黑名单
bool test_whitelist_blacklist() {
    TaskBuilder builder;
    
    builder.title("Restricted Task")
           .priority(50)
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           })
           .whitelist("claimer_1")
           .whitelist("claimer_2")
           .blacklist("claimer_3");
    
    auto task = builder.build();
    TEST_ASSERT(task != nullptr, "Build should succeed");
    
    auto whitelist = task->whitelist();
    TEST_ASSERT(whitelist.size() == 2, "Should have 2 whitelisted claimers");
    TEST_ASSERT(whitelist.find("claimer_1") != whitelist.end(), "Should have claimer_1 in whitelist");
    
    auto blacklist = task->blacklist();
    TEST_ASSERT(blacklist.size() == 1, "Should have 1 blacklisted claimer");
    TEST_ASSERT(blacklist.find("claimer_3") != blacklist.end(), "Should have claimer_3 in blacklist");
    
    return true;
}

// 测试 10: build_and_publish
bool test_build_and_publish() {
    TaskBuilder builder;
    
    builder.title("Published Task")
           .priority(50)
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           });
    
    auto task = builder.build_and_publish();
    TEST_ASSERT(task != nullptr, "Build and publish should succeed");
    TEST_ASSERT(task->status() == TaskStatus::Published, "Status should be Published");
    
    // 验证发布时间戳已设置
    auto published_at = task->published_at();
    TEST_ASSERT(published_at.time_since_epoch().count() > 0, "Published timestamp should be set");
    
    return true;
}

// 测试 11: auto_cleanup 标志（默认 false，且可设置为 true）
bool test_auto_cleanup_flag() {
    // 默认情况下，auto_cleanup 应为 false
    TaskBuilder default_builder;
    default_builder.title("Default AutoClean")
                   .priority(10)
                   .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
                       return TaskResult{true, "Done"};
                   });
    auto t_default = default_builder.build();
    TEST_ASSERT(t_default != nullptr, "Default build should succeed");
    TEST_ASSERT(t_default->auto_cleanup() == false, "Default auto_cleanup should be false");

    // 设置为 true
    TaskBuilder builder;
    builder.title("AutoClean Task")
           .priority(20)
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           })
           .auto_cleanup(true);

    auto t = builder.build();
    TEST_ASSERT(t != nullptr, "Build should succeed");
    TEST_ASSERT(t->auto_cleanup() == true, "auto_cleanup should be true when set");

    return true;
}

// 测试 11: reset 方法
bool test_reset() {
    TaskBuilder builder;
    
    builder.title("Task 1")
           .description("Description 1")
           .priority(50)
           .category("cat1")
           .add_tag("tag1")
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           });
    
    TEST_ASSERT(builder.is_valid(), "Builder should be valid before reset");
    
    builder.reset();
    
    TEST_ASSERT(!builder.is_valid(), "Builder should be invalid after reset");
    
    // 验证可以重新使用
    builder.title("Task 2")
           .priority(30)
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done 2"};
           });
    
    auto task = builder.build();
    TEST_ASSERT(task != nullptr, "Should be able to build after reset");
    TEST_ASSERT(task->title() == "Task 2", "New task should have new title");
    TEST_ASSERT(task->priority() == 30, "New task should have new priority");
    
    return true;
}

// 测试 12: 多次构建
bool test_multiple_builds() {
    TaskBuilder builder;
    
    builder.title("Template Task")
           .priority(50)
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           });
    
    auto task1 = builder.build();
    auto task2 = builder.build();
    
    TEST_ASSERT(task1 != nullptr, "First build should succeed");
    TEST_ASSERT(task2 != nullptr, "Second build should succeed");
    TEST_ASSERT(task1->id() != task2->id(), "Each build should create a new task with unique ID");
    TEST_ASSERT(task1->title() == task2->title(), "Tasks should have same title");
    
    return true;
}

// 测试 13: 描述长度验证
bool test_validation_description_length() {
    TaskBuilder builder;
    
    std::string long_description(15000, 'a');  // 15000个字符
    
    builder.title("Task with long description")
           .description(long_description)
           .handler([](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
               return TaskResult{true, "Done"};
           });
    
    auto errors = builder.validation_errors();
    bool found_desc_error = false;
    for (const auto &err : errors) {
        if (err.find("description") != std::string::npos && err.find("long") != std::string::npos) {
            found_desc_error = true;
            break;
        }
    }
    TEST_ASSERT(found_desc_error, "Should have description length error");
    
    return true;
}

// 测试 14: 完整的工作流
bool test_complete_workflow() {
    TaskBuilder builder;
    
    bool task_executed = false;
    std::string task_input;
    
    auto task = builder
        .title("Complete Workflow Task")
        .description("A task demonstrating the complete workflow")
        .priority(75)
        .category("workflow")
        .add_tag("demo")
        .add_tag("complete")
        .handler([&](Task &t, const std::string &input) -> tl::expected<TaskResult, std::string> {
            task_executed = true;
            task_input = input;
            t.set_progress(100);
            return TaskResult{true, "Workflow completed successfully"};
        })
        .metadata("project", "xswl-youdidit")
        .metadata("phase", "2")
        .build_and_publish();
    
    TEST_ASSERT(task != nullptr, "Task should be built successfully");
    TEST_ASSERT(task->status() == TaskStatus::Published, "Task should be published");
    
    // 模拟任务执行流程
    task->set_status(TaskStatus::Claimed);
    auto exec_result = task->execute("test_data");
    
    TEST_ASSERT(exec_result.has_value(), "Task execution should succeed");
    TEST_ASSERT(task_executed, "Task handler should be executed");
    TEST_ASSERT(task_input == "test_data", "Task handler should receive correct input");
    TEST_ASSERT(task->status() == TaskStatus::Completed, "Task should be completed");
    TEST_ASSERT(task->progress() == 100, "Task progress should be 100");
    
    return true;
}

// ========== 主函数 ==========
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      TaskBuilder Class Unit Tests     " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    bool all_passed = true;
    
    RUN_TEST(test_default_construction);
    RUN_TEST(test_fluent_api);
    RUN_TEST(test_validation_missing_title);
    RUN_TEST(test_validation_missing_handler);
    RUN_TEST(test_validation_priority_range);
    RUN_TEST(test_validation_title_length);
    RUN_TEST(test_build_success);
    RUN_TEST(test_build_failure);
    RUN_TEST(test_whitelist_blacklist);
    RUN_TEST(test_build_and_publish);
    RUN_TEST(test_reset);
    RUN_TEST(test_multiple_builds);
    RUN_TEST(test_validation_description_length);
    RUN_TEST(test_complete_workflow);
    
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

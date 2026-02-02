#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

using namespace xswl::youdidit;

// 测试：边界情况 - 空任务处理、空申领者等
int main() {
    auto platform = std::make_shared<TaskPlatform>("p");
    auto claimer = std::make_shared<Claimer>("c1", "C");
    platform->register_claimer(claimer);

    bool all_passed = true;

    // 测试1：尝试运行不存在的任务
    {
        TaskResult result = claimer->run_task("nonexistent", "input");
        if (result.ok()) {
            std::cerr << "Test 1 FAILED: Should fail for nonexistent task" << std::endl;
            all_passed = false;
        } else {
            std::cout << "Test 1 PASSED: Correctly rejected nonexistent task" << std::endl;
        }
    }

    // 测试2：尝试完成未申领的任务
    {
        auto task = std::make_shared<Task>("t1");
        task->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("ok"); });
        platform->publish_task(task);
        
        auto res = claimer->complete_task("t1", TaskResult("done"));
        if (res.has_value()) {
            std::cerr << "Test 2 FAILED: Should fail for unclaimed task" << std::endl;
            all_passed = false;
        } else {
            std::cout << "Test 2 PASSED: Correctly rejected unclaimed task completion" << std::endl;
        }
    }

    // 测试3：尝试申领已完成的任务
    {
        auto task = std::make_shared<Task>("t2");
        task->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("ok"); });
        platform->publish_task(task);
        
        auto claim1 = platform->claim_task(claimer, "t2");
        if (!claim1.has_value()) {
            std::cerr << "Test 3 setup FAILED" << std::endl;
            return 1;
        }
        
        claimer->run_task(task, "input");
        
        // 尝试再次申领已完成的任务
        auto claimer2 = std::make_shared<Claimer>("c2", "C2");
        platform->register_claimer(claimer2);
        
        auto claim2 = platform->claim_task(claimer2, "t2");
        if (claim2.has_value()) {
            std::cerr << "Test 3 FAILED: Should fail for completed task" << std::endl;
            all_passed = false;
        } else {
            std::cout << "Test 3 PASSED: Correctly rejected claiming completed task" << std::endl;
        }
    }

    // 测试4：空处理器的任务
    {
        auto task = std::make_shared<Task>("t3");
        // 不设置 handler
        platform->publish_task(task);
        
        auto claim = platform->claim_task(claimer, "t3");
        if (!claim.has_value()) {
            std::cerr << "Test 4 setup FAILED" << std::endl;
            return 1;
        }
        
        TaskResult result = task->execute("input");
        if (result.ok()) {
            std::cerr << "Test 4 FAILED: Should fail for task without handler" << std::endl;
            all_passed = false;
        } else {
            std::cout << "Test 4 PASSED: Correctly rejected task without handler" << std::endl;
        }
    }

    // 测试5：超过最大并发数
    {
        auto claimer3 = std::make_shared<Claimer>("c3", "C3");
        claimer3->set_max_concurrent(2);
        platform->register_claimer(claimer3);
        
        for (int i = 0; i < 3; ++i) {
            auto task = std::make_shared<Task>("t_max_" + std::to_string(i));
            task->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("ok"); });
            platform->publish_task(task);
        }
        
        auto claim1 = platform->claim_task(claimer3, "t_max_0");
        auto claim2 = platform->claim_task(claimer3, "t_max_1");
        auto claim3 = platform->claim_task(claimer3, "t_max_2");
        
        if (!claim1.has_value() || !claim2.has_value()) {
            std::cerr << "Test 5 setup FAILED" << std::endl;
            all_passed = false;
        } else if (claim3.has_value()) {
            std::cerr << "Test 5 FAILED: Should reject claim over max concurrent" << std::endl;
            all_passed = false;
        } else {
            std::cout << "Test 5 PASSED: Correctly enforced max concurrent limit" << std::endl;
        }
    }

    if (all_passed) {
        std::cout << "\nAll edge case tests PASSED" << std::endl;
        return 0;
    } else {
        std::cerr << "\nSome edge case tests FAILED" << std::endl;
        return 1;
    }
}

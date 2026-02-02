#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

using namespace xswl::youdidit;

/**
 * @brief 测试同一任务的并发执行保护
 * 
 * 验证多个线程同时调用 run_task 执行同一任务时，
 * 只有一个线程能成功执行，其他线程会收到错误。
 */
int main() {
    // 创建平台
    auto platform = std::make_shared<TaskPlatform>("test-platform");
    
    // 创建申领者
    auto claimer = std::make_shared<Claimer>("claimer-1", "Test Claimer");
    platform->register_claimer(claimer);
    
    // 创建一个耗时任务
    auto task = std::make_shared<Task>("task-1");
    task->set_title("Concurrent Test Task");
    task->set_handler([](Task& /*task*/, const std::string& input) -> TaskResult {
        // 模拟耗时操作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        TaskResult result;
        result.output = "Completed: " + input;
        return result;
    });
    
    // 将任务发布到平台并申领
    platform->publish_task(task);
    auto claim_result = platform->claim_task(claimer, "task-1");
    
    if (!claim_result.has_value()) {
        std::cerr << "Failed to claim task: " << claim_result.error().message << std::endl;
        return 1;
    }
    
    // 统计执行结果
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    std::atomic<int> concurrent_error_count{0};
    
    // 创建多个线程同时执行同一任务
    const int thread_count = 8;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, i]() {
            TaskResult result = claimer->run_task(task, "input-" + std::to_string(i));
            
            if (result.ok()) {
                success_count.fetch_add(1);
                std::cout << "Thread " << i << " succeeded" << std::endl;
            } else {
                error_count.fetch_add(1);
                std::cout << "Thread " << i << " failed: " << result.error.message << std::endl;
                
                // 检查是否是并发执行错误
                if (result.error.message.find("already being executed") != std::string::npos) {
                    concurrent_error_count.fetch_add(1);
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    // 验证结果
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Threads count: " << thread_count << std::endl;
    std::cout << "Success count: " << success_count.load() << std::endl;
    std::cout << "Error count: " << error_count.load() << std::endl;
    std::cout << "Concurrent error count: " << concurrent_error_count.load() << std::endl;
    
    // 预期结果：只有一个线程成功，其他线程因并发保护而失败
    bool test_passed = (success_count.load() == 1) && 
                       (concurrent_error_count.load() == thread_count - 1);
    
    if (test_passed) {
        std::cout << "\nTest PASSED: Concurrent execution protection works correctly" << std::endl;
        return 0;
    } else {
        std::cerr << "\nTest FAILED: Expected 1 success and " << (thread_count - 1) 
                  << " concurrent errors" << std::endl;
        return 1;
    }
}

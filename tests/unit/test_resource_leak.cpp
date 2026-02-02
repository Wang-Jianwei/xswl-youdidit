#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <chrono>

using namespace xswl::youdidit;

// 测试：资源泄漏检测 - 申领后未完成/放弃的任务
int main() {
    const int N_TASKS = 100;
    auto platform = std::make_shared<TaskPlatform>("p");
    auto claimer = std::make_shared<Claimer>("c1", "C");
        claimer->set_max_concurrent(N_TASKS); // 设置足够大的并发数
    platform->register_claimer(claimer);

    // 创建任务
    for (int i = 0; i < N_TASKS; ++i) {
        auto t = std::make_shared<Task>("t" + std::to_string(i));
        t->set_title("leak test");
        t->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("ok"); });
        platform->publish_task(t);
    }

    // 申领所有任务但不完成
    for (int i = 0; i < N_TASKS; ++i) {
        platform->claim_task(claimer, "t" + std::to_string(i));
    }

    // 验证申领计数
    if (claimer->active_task_count() != N_TASKS) {
        std::cerr << "Active task count should be " << N_TASKS 
                  << ", got: " << claimer->active_task_count() << std::endl;
        return 1;
    }

    // 放弃所有任务
    for (int i = 0; i < N_TASKS; ++i) {
        claimer->abandon_task("t" + std::to_string(i), "cleanup");
    }

    // 验证清理后计数
    if (claimer->active_task_count() != 0) {
        std::cerr << "Active task count should be 0 after abandon, got: " 
                  << claimer->active_task_count() << std::endl;
        return 1;
    }

    if (claimer->total_abandoned() != N_TASKS) {
        std::cerr << "Total abandoned should be " << N_TASKS 
                  << ", got: " << claimer->total_abandoned() << std::endl;
        return 1;
    }

    std::cout << "PASSED" << std::endl;
    return 0;
}

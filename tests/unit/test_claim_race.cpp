#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

using namespace xswl::youdidit;

// 测试：并发 claim_task 竞争（原子性验证）
int main() {
    const int N_THREADS = 20;
    auto platform = std::make_shared<TaskPlatform>("p");

    // 创建单个任务
    auto task = std::make_shared<Task>("t1");
    task->set_title("claim race");
    task->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("ok"); });
    platform->publish_task(task);

    // 创建多个申领者
    std::vector<std::shared_ptr<Claimer>> claimers;
    for (int i = 0; i < N_THREADS; ++i) {
        auto c = std::make_shared<Claimer>("c" + std::to_string(i), "C");
        platform->register_claimer(c);
        claimers.push_back(c);
    }

    std::atomic<int> success_count{0};
    std::atomic<int> fail_count{0};

    // 所有线程同时尝试申领同一任务
    std::vector<std::thread> threads;
    for (int i = 0; i < N_THREADS; ++i) {
        threads.emplace_back([&, i]() {
            auto res = platform->claim_task(claimers[i], "t1");
            if (res.has_value()) {
                success_count.fetch_add(1);
            } else {
                fail_count.fetch_add(1);
            }
        });
    }

    for (auto &t : threads) t.join();

    // 验证：只有一个线程成功申领
    if (success_count.load() != 1) {
        std::cerr << "Success count should be 1, got: " << success_count.load() << std::endl;
        return 1;
    }

    if (fail_count.load() != N_THREADS - 1) {
        std::cerr << "Fail count should be " << (N_THREADS - 1) << ", got: " << fail_count.load() << std::endl;
        return 1;
    }

    // 验证任务状态为 Claimed
    if (task->status() != TaskStatus::Claimed) {
        std::cerr << "Task status should be Claimed, got: " << (int)task->status() << std::endl;
        return 1;
    }

    std::cout << "PASSED" << std::endl;
    return 0;
}

#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

using namespace xswl::youdidit;

// 测试：同时调用 Task::complete、Claimer::complete_task、abandon_task 的竞争
int main() {
    auto platform = std::make_shared<TaskPlatform>("p");
    auto claimer = std::make_shared<Claimer>("c1", "C");
    platform->register_claimer(claimer);

    auto task = std::make_shared<Task>("t1");
    task->set_title("finalize race");
    task->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("ok"); });

    platform->publish_task(task);
    auto claim = platform->claim_task(claimer, "t1");
    if (!claim.has_value()) {
        std::cerr << "claim failed" << std::endl;
        return 1;
    }

    // 将任务转为 Processing 状态
    task->start();

    std::atomic<int> complete_success{0};
    std::atomic<int> abandon_success{0};
    std::atomic<int> fail_count{0};

    // 多个线程尝试完成或放弃任务
    std::vector<std::thread> threads;
    
    // 5个线程尝试 complete
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&]() {
            auto res = task->complete(TaskResult("done"));
            if (res.has_value()) {
                complete_success.fetch_add(1);
            } else {
                fail_count.fetch_add(1);
            }
        });
    }

    // 5个线程尝试 abandon
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&]() {
            auto res = task->abandon("reason");
            if (res.has_value()) {
                abandon_success.fetch_add(1);
            } else {
                fail_count.fetch_add(1);
            }
        });
    }

    for (auto &t : threads) t.join();

    // 验证：只有一个操作成功（complete 或 abandon）
    int total_success = complete_success.load() + abandon_success.load();
    if (total_success != 1) {
        std::cerr << "Total success should be 1, got: " << total_success 
                  << " (complete: " << complete_success.load() 
                  << ", abandon: " << abandon_success.load() << ")" << std::endl;
        return 1;
    }

    // 验证任务状态为终态
    TaskStatus status = task->status();
    if (status != TaskStatus::Completed && status != TaskStatus::Abandoned) {
        std::cerr << "Task should be in terminal state, got: " << (int)status << std::endl;
        return 1;
    }

    std::cout << "Terminal state: " << (status == TaskStatus::Completed ? "Completed" : "Abandoned") << std::endl;
    std::cout << "PASSED" << std::endl;
    return 0;
}

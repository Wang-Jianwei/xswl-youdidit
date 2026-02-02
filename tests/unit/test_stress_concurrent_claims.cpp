#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

using namespace xswl::youdidit;

int main() {
    const int N_TASKS = 400;
    const int N_CLAIMERS = 10;

    std::cout << "Tasks=" << N_TASKS << std::endl;
    std::cout << "Claimers=" << N_CLAIMERS << std::endl;

    auto platform = std::make_shared<TaskPlatform>("p");

    // 创建任务
    for (int i = 0; i < N_TASKS; ++i) {
        auto t = std::make_shared<Task>("t" + std::to_string(i));
        t->set_title("stress");
        t->set_handler([](Task& /*t*/, const std::string&){
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            return TaskResult("ok");
        });
        platform->publish_task(t);
    }

    // 创建并注册申领者
    std::vector<std::shared_ptr<Claimer>> claimers;
    for (int i = 0; i < N_CLAIMERS; ++i) {
        auto c = std::make_shared<Claimer>("c" + std::to_string(i), "C");
        platform->register_claimer(c);
        claimers.push_back(c);
    }

    // 每个申领者一个线程，循环 claim_next_task 并执行直到没有任务
    std::vector<std::thread> threads;
    for (int i = 0; i < N_CLAIMERS; ++i) {
        threads.emplace_back([&, i]() {
            auto claimer = claimers[i];
            while (true) {
                auto res = platform->claim_next_task(claimer);
                if (!res.has_value()) {
                    // 结束条件：没有可申领任务
                    break;
                }
                auto task = res.value();
                // 执行任务（内部会调用 complete_task）
                claimer->run_task(task, "input");
            }
        });
    }

    for (auto &t : threads) t.join();

    // 验证
    size_t completed = platform->task_count_by_status(TaskStatus::Completed);
    if ((int)completed != N_TASKS) {
        std::cerr << "Completed mismatch: " << completed << " != " << N_TASKS << std::endl;
        return 1;
    }

    // 所有 claimer 的 total_completed 之和应为 N_TASKS
    uint64_t sum = 0;
    for (const auto &c : claimers) sum += c->total_completed();
    if ((int)sum != N_TASKS) {
        std::cerr << "Sum total_completed mismatch: " << sum << " != " << N_TASKS << std::endl;
        return 1;
    }

    // 活跃计数应为 0
    for (const auto &c : claimers) {
        if (c->active_task_count() != 0) {
            std::cerr << "Claimer " << c->id() << " active_task_count != 0: " << c->active_task_count() << std::endl;
            return 1;
        }
    }

    std::cout << "PASSED" << std::endl;
    return 0;
}

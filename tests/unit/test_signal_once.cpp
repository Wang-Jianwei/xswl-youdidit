#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

using namespace xswl::youdidit;

int main() {
    auto platform = std::make_shared<TaskPlatform>("p");
    auto claimer = std::make_shared<Claimer>("c1", "C");
    platform->register_claimer(claimer);

    auto task = std::make_shared<Task>("t1");
    task->set_title("t");
    task->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("ok"); });

    platform->publish_task(task);
    auto claim = platform->claim_task(claimer, "t1");
    if (!claim.has_value()) { std::cerr << "claim failed" << std::endl; return 1; }

    std::atomic<int> claimer_sig_count{0};
    std::atomic<int> task_sig_count{0};

    claimer->sig_task_completed.connect([&](Claimer &, std::shared_ptr<Task>, const TaskResult &) {
        claimer_sig_count.fetch_add(1);
    });
    task->sig_completed.connect([&](Task &, const TaskResult &) {
        task_sig_count.fetch_add(1);
    });

    const int THREADS = 8;
    std::vector<std::thread> threads;
    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&]() {
            claimer->complete_task("t1", TaskResult("done"));
        });
    }
    for (auto &t : threads) t.join();

    if (claimer_sig_count.load() != 1) {
        std::cerr << "claimer_sig_count != 1: " << claimer_sig_count.load() << std::endl;
        return 1;
    }
    if (task_sig_count.load() != 1) {
        std::cerr << "task_sig_count != 1: " << task_sig_count.load() << std::endl;
        return 1;
    }

    std::cout << "PASSED" << std::endl;
    return 0;
}

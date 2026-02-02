#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

using namespace xswl::youdidit;

bool test_complete_idempotent() {
    auto platform = std::make_shared<TaskPlatform>("p");
    auto claimer = std::make_shared<Claimer>("c1", "C");
    platform->register_claimer(claimer);

    auto task = std::make_shared<Task>("t1");
    task->set_title("t");
    task->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("ok"); });

    platform->publish_task(task);
    auto claim = platform->claim_task(claimer, "t1");
    if (!claim.has_value()) { std::cerr << "claim failed" << std::endl; return false; }

    const int THREADS = 8;
    std::vector<std::thread> threads;
    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&, i](){
            claimer->complete_task("t1", TaskResult("done"));
        });
    }
    for (auto &t : threads) t.join();

    // 只有一个完成计数
    if (claimer->total_completed() != 1) {
        std::cerr << "total_completed != 1: " << claimer->total_completed() << std::endl;
        return false;
    }
    if (claimer->active_task_count() != 0) {
        std::cerr << "active_task_count != 0: " << claimer->active_task_count() << std::endl;
        return false;
    }
    return true;
}

bool test_abandon_idempotent() {
    auto platform = std::make_shared<TaskPlatform>("p");
    auto claimer = std::make_shared<Claimer>("c2", "C");
    platform->register_claimer(claimer);

    auto task = std::make_shared<Task>("t2");
    task->set_title("t");
    task->set_handler([](Task& /*t*/, const std::string&){ return TaskResult("fail"); });

    platform->publish_task(task);
    auto claim = platform->claim_task(claimer, "t2");
    if (!claim.has_value()) { std::cerr << "claim failed" << std::endl; return false; }

    const int THREADS = 8;
    std::vector<std::thread> threads;
    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&, i](){
            claimer->abandon_task("t2", "reason");
        });
    }
    for (auto &t : threads) t.join();

    if (claimer->total_abandoned() != 1) {
        std::cerr << "total_abandoned != 1: " << claimer->total_abandoned() << std::endl;
        return false;
    }
    if (claimer->active_task_count() != 0) {
        std::cerr << "active_task_count != 0: " << claimer->active_task_count() << std::endl;
        return false;
    }
    return true;
}

int main() {
    bool all = true;
    std::cout << "Running test_complete_idempotent... ";
    if (test_complete_idempotent()) std::cout << "PASSED\n"; else { std::cout << "FAILED\n"; all = false; }
    std::cout << "Running test_abandon_idempotent... ";
    if (test_abandon_idempotent()) std::cout << "PASSED\n"; else { std::cout << "FAILED\n"; all = false; }
    return all ? 0 : 1;
}

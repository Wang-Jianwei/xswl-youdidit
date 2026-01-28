#include <xswl/youdidit/core/task_platform.hpp>
#include <cassert>
#include <atomic>
#include <thread>
#include <vector>
#include <iostream>

using namespace xswl::youdidit;

int main() {
    std::cout << "Thread safety test: concurrent claim same task... " << std::flush;

    TaskPlatform platform;
    auto task = platform.task_builder()
                    .title("Concurrent Task")
                    .category("general")
                    .priority(50)
                    .handler([](Task &, const std::string &) {
                        return TaskResult(true, "ok");
                    })
                    .build_and_publish();
    platform.publish_task(task);

    const int thread_count = 8;
    std::atomic<int> success_count{0};

    std::vector<std::shared_ptr<Claimer>> claimers;
    claimers.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        auto claimer = std::make_shared<Claimer>("claimer-" + std::to_string(i), "Worker" + std::to_string(i));
        claimer->add_category("general");
        platform.register_claimer(claimer);
        claimers.push_back(claimer);
    }

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, i]() {
            auto result = claimers[i]->claim_task(task->id());
            if (result.has_value()) {
                success_count.fetch_add(1, std::memory_order_acq_rel);
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    assert(success_count.load(std::memory_order_acquire) == 1);
    assert(task->status() == TaskStatus::Claimed);
    assert(!task->claimer_id().empty());

    std::cout << "PASSED" << std::endl;
    return 0;
}

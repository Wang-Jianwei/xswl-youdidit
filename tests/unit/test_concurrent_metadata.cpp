#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <random>

using namespace xswl::youdidit;

// 测试：并发修改任务元数据和标签，同时执行任务
int main() {
    auto platform = std::make_shared<TaskPlatform>("p");
    auto claimer = std::make_shared<Claimer>("c1", "C");
    platform->register_claimer(claimer);

    auto task = std::make_shared<Task>("t1");
    task->set_title("metadata test");
    task->set_handler([](Task& t, const std::string&) {
        // 在执行期间读取元数据和标签
        auto meta = t.metadata();
        auto tags = t.tags();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return TaskResult("ok");
    });

    platform->publish_task(task);
    auto claim = platform->claim_task(claimer, "t1");
    if (!claim.has_value()) {
        std::cerr << "claim failed" << std::endl;
        return 1;
    }

    std::atomic<bool> stop{false};
    std::atomic<int> modifications{0};

    // 一个线程执行任务
    std::thread executor([&]() {
        claimer->run_task(task, "input");
        stop.store(true);
    });

    // 多个线程并发修改元数据和标签
    std::vector<std::thread> modifiers;
    for (int i = 0; i < 4; ++i) {
        modifiers.emplace_back([&, i]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 10);
            
            while (!stop.load()) {
                // 随机修改元数据
                task->set_metadata("key" + std::to_string(i), "value" + std::to_string(dis(gen)));
                
                // 随机添加/删除标签
                if (dis(gen) % 2 == 0) {
                    task->add_tag("tag" + std::to_string(i));
                } else {
                    task->remove_tag("tag" + std::to_string(i));
                }
                
                modifications.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    executor.join();
    for (auto &m : modifiers) m.join();

    // 验证任务成功完成
    if (task->status() != TaskStatus::Completed) {
        std::cerr << "Task not completed, status: " << (int)task->status() << std::endl;
        return 1;
    }

    std::cout << "Modifications during execution: " << modifications.load() << std::endl;
    std::cout << "PASSED" << std::endl;
    return 0;
}

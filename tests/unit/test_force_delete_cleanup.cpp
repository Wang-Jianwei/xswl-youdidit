#include <xswl/youdidit/core/task_platform.hpp>
#include <xswl/youdidit/core/task.hpp>
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

    // 强制删除
    bool removed = platform->remove_task("t1", true);
    if (!removed) { std::cerr << "remove failed" << std::endl; return 1; }

    // 平台应不再包含任务
    if (platform->has_task("t1")) { std::cerr << "platform still has task" << std::endl; return 1; }

    // Claimer 不应再认为自己持有该任务
    if (claimer->has_task("t1")) { std::cerr << "claimer still has task" << std::endl; return 1; }

    if (claimer->claimed_task_count() != 0) { std::cerr << "claimed_task_count != 0" << std::endl; return 1; }

    std::cout << "PASSED" << std::endl;
    return 0;
}

#include <xswl/youdidit/youdidit.hpp>
#include <iostream>
#include <vector>

using namespace xswl::youdidit;

std::shared_ptr<Task> publish_task(TaskPlatform &platform,
                                   const std::string &title,
                                   const std::string &category,
                                   int priority) {
    auto builder = platform.task_builder();
    builder.title(title)
           .description("multi claimer example")
           .priority(priority)
           .category(category)
           .handler([](Task & /*task*/, const std::string &input) -> tl::expected<TaskResult, std::string> {
               TaskResult r(true, "done: " + input);
               return r;
           });
    auto task = builder.build();
    platform.publish_task(task);
    return task;
}

int main() {
    TaskPlatform platform;

    auto alice = std::make_shared<Claimer>("alice", "Alice");
    alice->add_category("data");
    alice->set_max_concurrent(2);

    auto bob = std::make_shared<Claimer>("bob", "Bob");
    bob->add_category("ops");
    bob->set_max_concurrent(1);

    platform.register_claimer(alice);
    platform.register_claimer(bob);

    publish_task(platform, "Data Cleanup", "data", 70);
    publish_task(platform, "ETL Job", "data", 60);
    publish_task(platform, "Server Restart", "ops", 90);

    auto alice_claimed = alice->claim_tasks_to_capacity();
    auto bob_claimed = bob->claim_tasks_to_capacity();

    std::cout << "Alice claimed " << alice_claimed.size() << " tasks" << std::endl;
    for (const auto &task : alice_claimed) {
        alice->run_task(task, "alice-run");
    }

    std::cout << "Bob claimed " << bob_claimed.size() << " tasks" << std::endl;
    for (const auto &task : bob_claimed) {
        bob->run_task(task, "bob-run");
    }

    auto stats = platform.get_statistics();
    std::cout << "Total tasks: " << stats.total_tasks << ", completed: " << stats.completed_tasks << std::endl;
    return 0;
}

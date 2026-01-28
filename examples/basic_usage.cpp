#include <xswl/youdidit/youdidit.hpp>
#include <iostream>

using namespace xswl::youdidit;

int main() {
    TaskPlatform platform;

    auto claimer = std::make_shared<Claimer>("claimer-1", "Alice");
    claimer->add_category("default");
    platform.register_claimer(claimer);

    auto builder = platform.task_builder();
    builder.title("Hello Task")
           .description("Minimal end-to-end flow")
           .priority(50)
           .category("default")
           .handler([](Task & /*task*/, const std::string &input) -> tl::expected<TaskResult, std::string> {
               TaskResult result(true, "handled: " + input);
               result.output = input;
               return result;
           });

    auto task = builder.build();
    platform.publish_task(task);

    auto claimed = claimer->claim_next_task();
    if (!claimed.has_value()) {
        std::cerr << "Failed to claim task: " << claimed.error().message << std::endl;
        return 1;
    }

    auto result = claimer->run_task(claimed.value()->id(), "payload");
    if (!result.has_value()) {
        std::cerr << "Execution failed: " << result.error().message << std::endl;
        return 1;
    }

    std::cout << "Task " << claimed.value()->id() << " completed with summary: "
              << result.value().summary << std::endl;
    std::cout << "Platform completed tasks: " << platform.get_statistics().completed_tasks << std::endl;
    return 0;
}

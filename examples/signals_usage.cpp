#include <iostream>
#include <memory>
#include <xswl/signals.hpp>
#include <xswl/youdidit/youdidit.hpp>

using namespace xswl::youdidit;

int main() {
    TaskPlatform platform("platform-signals");

    // Logger for platform events
    struct Logger {
        void on_published(const std::shared_ptr<Task> &t) {
            std::cout << "[Platform] published: " << t->id() << "\n";
        }
        void on_completed(const std::shared_ptr<Task> &t, const TaskResult &r) {
            std::cout << "[Platform] completed: " << t->id() << " -> " << r.summary << "\n";
        }
    } logger;

    // Connect platform signals
    platform.sig_task_published.connect(&logger, &Logger::on_published, 100);
    platform.sig_task_completed.connect(&logger, &Logger::on_completed);

    // Create a claimer and connect signals
    auto claimer = std::make_shared<Claimer>("worker-1", "Worker One");
    claimer->sig_task_claimed.connect([](Claimer &c, std::shared_ptr<Task> t){
        std::cout << "[Claimer] " << c.id() << " claimed " << t->id() << "\n";
    });
    claimer->sig_task_started.connect([](Claimer &c, std::shared_ptr<Task> t){
        std::cout << "[Claimer] " << c.id() << " started " << t->id() << "\n";
    });
    claimer->sig_task_completed.connect([](Claimer &c, std::shared_ptr<Task> t, const TaskResult &res){
        std::cout << "[Claimer] " << c.id() << " completed " << t->id() << " -> " << res.summary << "\n";
    });

    platform.register_claimer(claimer);

    // Build a task and attach task-local signal handlers
    TaskBuilder builder = platform.task_builder();
    builder.title("Signals Demo")
           .description("Demonstrate Task/Claimer/Platform signals")
           .category("default")
           .priority(50)
           .handler([](Task & /*task*/, const std::string &input) -> TaskResult {
               // Simulate progress updates
               // Note: here we just return a result; Task::set_progress might be used inside real handler
               TaskResult r(std::string("processed: ") + input);
               r.output = input;
               return r;
           });

    auto task = builder.build();

    task->sig_status_changed.connect([](Task &t, TaskStatus old_s, TaskStatus new_s){
        std::cout << "[Task] " << t.id() << " status: " << to_string(old_s) << " -> " << to_string(new_s) << "\n";
    });

    task->sig_progress_updated.connect([](Task &t, int p){
        std::cout << "[Task] " << t.id() << " progress: " << p << "%\n";
    });

    task->sig_cancel_requested.connect([](Task &t, const std::string &reason){
        std::cout << "[Task] cancel requested for " << t.id() << ": " << reason << "\n";
    });

    task->sig_completed.connect([](Task &t, const TaskResult &r){
        std::cout << "[Task] " << t.id() << " completed (local handler) -> " << r.summary << "\n";
    });

    // Publish the task to platform (this should trigger platform.sig_task_published)
    platform.publish_task(task);

    // Claimer claims next task and runs it
    auto claimed = claimer->claim_next_task();
    if (!claimed.has_value()) {
        std::cerr << "Claim failed: " << claimed.error().message << "\n";
        return 1;
    }

    auto res = claimer->run_task(claimed.value(), "payload-123");
    if (!res.ok()) {
        std::cerr << "Run failed: " << res.error.message << "\n";
        return 1;
    }

    std::cout << "Main: Task " << claimed.value()->id() << " finished with summary: " << res.summary << "\n";

    // Demonstrate task-level cancel request (won't affect completed task, but shows signal)
    task->request_cancel("unneeded");

    return 0;
}

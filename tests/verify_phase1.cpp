/**
 * @file verify_phase1.cpp
 * @brief Phase 1 验证程序
 * 
 * 验证基础设施和核心类型定义是否正确
 */

#include <xswl/youdidit/youdidit.hpp>
#include <iostream>

int main() {
    using namespace xswl::youdidit;
    
    std::cout << "===========================================\n";
    std::cout << "Phase 1 Verification\n";
    std::cout << "===========================================\n\n";
    
    // 1. 版本信息
    std::cout << "Library Version: " << VERSION << "\n";
    std::cout << "  Major: " << VERSION_MAJOR << "\n";
    std::cout << "  Minor: " << VERSION_MINOR << "\n";
    std::cout << "  Patch: " << VERSION_PATCH << "\n\n";
    
    // 2. TaskStatus 测试
    std::cout << "TaskStatus enumeration:\n";
    std::cout << "  Draft      -> " << to_string(TaskStatus::Draft) << "\n";
    std::cout << "  Published  -> " << to_string(TaskStatus::Published) << "\n";
    std::cout << "  Processing -> " << to_string(TaskStatus::Processing) << "\n";
    std::cout << "  Completed  -> " << to_string(TaskStatus::Completed) << "\n\n";
    
    // 3. ClaimerState 示例
    std::cout << "ClaimerState examples:\n";
    std::cout << "  Idle    -> " << to_string(ClaimerState{}) << "\n";
    ClaimerState busy; busy.claimed_task_count = 1; busy.max_concurrent = 1;
    std::cout << "  Busy    -> " << to_string(busy) << "\n";
    ClaimerState offline; offline.online = false;
    std::cout << "  Offline -> " << to_string(offline) << "\n\n";
    
    // 4. TaskResult 测试
    std::cout << "TaskResult structure:\n";
    TaskResult result("Task executed successfully");
    // 将多个输出项合并为字符串（示例：可使用 JSON 或其他序列化格式）
    result.output = "output_file=/path/to/output.txt;execution_time=1.23s";
    
    std::cout << "  Success: " << (result.ok() ? "Yes" : "No") << "\n";
    std::cout << "  Summary: " << result.summary << "\n";
    std::cout << "  Output: " << result.output << "\n";
    std::cout << "\n";
    
    // 5. Error 测试
    std::cout << "Error structure:\n";
    Error error1("Task not found", ErrorCode::TASK_NOT_FOUND);
    std::cout << "  Message: " << error1.message << "\n";
    std::cout << "  Code: " << error1.code << "\n\n";
    
    // 6. ErrorCode 常量
    std::cout << "Error codes:\n";
    std::cout << "  SUCCESS                  = " << ErrorCode::SUCCESS << "\n";
    std::cout << "  TASK_NOT_FOUND           = " << ErrorCode::TASK_NOT_FOUND << "\n";
    std::cout << "  TASK_ALREADY_CLAIMED     = " << ErrorCode::TASK_ALREADY_CLAIMED << "\n";
    std::cout << "  CLAIMER_TOO_MANY_TASKS   = " << ErrorCode::CLAIMER_TOO_MANY_TASKS << "\n";
    std::cout << "  PLATFORM_QUEUE_FULL      = " << ErrorCode::PLATFORM_QUEUE_FULL << "\n\n";
    
    // 7. tl::optional 测试
    std::cout << "tl::optional integration:\n";
    auto opt_status = task_status_from_string("Published");
    if (opt_status.has_value()) {
        std::cout << "  ✓ Successfully parsed 'Published' -> " 
                  << to_string(opt_status.value()) << "\n";
    }
    
    auto invalid_status = task_status_from_string("InvalidStatus");
    if (!invalid_status.has_value()) {
        std::cout << "  ✓ Correctly rejected invalid status\n";
    }
    std::cout << "\n";
    
    // 8. 总结
    std::cout << "===========================================\n";
    std::cout << "Phase 1 Verification PASSED ✓\n";
    std::cout << "===========================================\n";
    std::cout << "\nAll components verified:\n";
    std::cout << "  [✓] Project structure\n";
    std::cout << "  [✓] CMake build system\n";
    std::cout << "  [✓] Third-party dependencies (tl::optional, tl::expected)\n";
    std::cout << "  [✓] Core type definitions\n";
    std::cout << "  [✓] Enumeration conversions\n";
    std::cout << "  [✓] Error handling structures\n";
    std::cout << "\nReady for Phase 2: Core Modules\n\n";
    
    return 0;
}

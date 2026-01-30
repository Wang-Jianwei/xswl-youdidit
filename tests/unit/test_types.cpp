/**
 * @file test_types.cpp
 * @brief 核心类型单元测试
 * 
 * 注意：完整的测试将在 Phase 5 使用 Google Test 框架实现
 * 这里先提供基本的测试框架
 */

#include <xswl/youdidit/core/types.hpp>
#include <iostream>
#include <cassert>

using namespace xswl::youdidit;

void test_task_status_to_string() {
    assert(to_string(TaskStatus::Draft) == "Draft");
    assert(to_string(TaskStatus::Published) == "Published");
    assert(to_string(TaskStatus::Claimed) == "Claimed");
    assert(to_string(TaskStatus::Processing) == "Processing");
    assert(to_string(TaskStatus::Paused) == "Paused");
    assert(to_string(TaskStatus::Completed) == "Completed");
    assert(to_string(TaskStatus::Failed) == "Failed");
    assert(to_string(TaskStatus::Cancelled) == "Cancelled");
    assert(to_string(TaskStatus::Abandoned) == "Abandoned");
    std::cout << "✓ test_task_status_to_string passed" << std::endl;
}

void test_task_status_from_string() {
    auto status = task_status_from_string("Published");
    assert(status.has_value());
    assert(status.value() == TaskStatus::Published);
    
    auto invalid = task_status_from_string("InvalidStatus");
    assert(!invalid.has_value());
    
    std::cout << "✓ test_task_status_from_string passed" << std::endl;
}

void test_claimer_status_to_string() {
    assert(to_string(ClaimerStatus::Idle) == "Idle");
    assert(to_string(ClaimerStatus::Busy) == "Busy");
    assert(to_string(ClaimerStatus::Offline) == "Offline");
    assert(to_string(ClaimerStatus::Paused) == "Paused");
    std::cout << "✓ test_claimer_status_to_string passed" << std::endl;
}

void test_claimer_status_from_string() {
    auto status = claimer_status_from_string("Idle");
    assert(status.has_value());
    assert(status.value() == ClaimerStatus::Idle);
    
    auto invalid = claimer_status_from_string("InvalidStatus");
    assert(!invalid.has_value());
    
    std::cout << "✓ test_claimer_status_from_string passed" << std::endl;
}

void test_task_result() {
    // 默认构造（表示成功）
    TaskResult r1;
    assert(r1.ok() == true);
    assert(r1.summary.empty());
    assert(r1.error.code == ErrorCode::SUCCESS);
    
    // 带摘要构造（表示成功）
    TaskResult r2("Task completed successfully");
    assert(r2.ok() == true);
    assert(r2.summary == "Task completed successfully");
    
    // 添加输出数据（字符串形式）
    r2.output = "/path/to/result.txt";
    assert(r2.output == "/path/to/result.txt");
    
    // 构造失败结果并设置错误信息
    TaskResult r3 = Error("Task failed", ErrorCode::TASK_EXECUTION_FAILED);
    r3.error.message = "Connection timeout";
    assert(r3.ok() == false);
    assert(r3.error.message == "Connection timeout");
    
    std::cout << "✓ test_task_result passed" << std::endl;
}

void test_error() {
    Error e1("Task not found");
    assert(e1.message == "Task not found");
    assert(e1.code_value() == 0);
    
    Error e2("Task already claimed", ErrorCode::TASK_ALREADY_CLAIMED);
    assert(e2.message == "Task already claimed");
    assert(e2.code == ErrorCode::TASK_ALREADY_CLAIMED);
    assert(e2.code_value() == 1003);
    
    std::cout << "✓ test_error passed" << std::endl;
}

void test_error_codes() {
    // 验证错误码定义
    assert(to_int(ErrorCode::SUCCESS) == 0);
    assert(to_int(ErrorCode::TASK_NOT_FOUND) == 1001);
    assert(to_int(ErrorCode::TASK_STATUS_INVALID) == 1002);
    assert(to_int(ErrorCode::TASK_ALREADY_CLAIMED) == 1003);
    assert(to_int(ErrorCode::TASK_CATEGORY_MISMATCH) == 1004);
    assert(to_int(ErrorCode::CLAIMER_NOT_FOUND) == 2001);
    assert(to_int(ErrorCode::CLAIMER_TOO_MANY_TASKS) == 2002);
    assert(to_int(ErrorCode::CLAIMER_ROLE_MISMATCH) == 2003);
    assert(to_int(ErrorCode::CLAIMER_BLOCKED) == 2004);
    assert(to_int(ErrorCode::CLAIMER_NOT_ALLOWED) == 2005);
    assert(to_int(ErrorCode::PLATFORM_QUEUE_FULL) == 3001);
    assert(to_int(ErrorCode::PLATFORM_NO_AVAILABLE_TASK) == 3002);
    
    std::cout << "✓ test_error_codes passed" << std::endl;
}

int main() {
    std::cout << "Running types unit tests..." << std::endl;
    std::cout << "===========================================" << std::endl;
    
    test_task_status_to_string();
    test_task_status_from_string();
    test_claimer_status_to_string();
    test_claimer_status_from_string();
    test_task_result();
    test_error();
    test_error_codes();
    
    std::cout << "===========================================" << std::endl;
    std::cout << "All tests passed! ✓" << std::endl;
    
    return 0;
}

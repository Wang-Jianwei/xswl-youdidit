# Google Test 依赖
# 注意：需要先安装 Google Test 或使用 FetchContent
# 这里提供基本框架，实际测试将在 Phase 5 实现

# 单元测试 - 简单的独立测试（不依赖 Google Test）
# test_types
add_executable(test_types unit/test_types.cpp)
set_target_properties(test_types PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_types")
target_link_libraries(test_types youdidit)

# test_type_traits (compile-time checks)
add_executable(test_type_traits unit/test_type_traits.cpp)
set_target_properties(test_type_traits PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_type_traits")
target_link_libraries(test_type_traits youdidit)

# test_task
add_executable(test_task unit/test_task.cpp)
set_target_properties(test_task PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_task")
target_link_libraries(test_task youdidit Threads::Threads)

# test_task_builder
add_executable(test_task_builder unit/test_task_builder.cpp)
set_target_properties(test_task_builder PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_task_builder")
target_link_libraries(test_task_builder youdidit Threads::Threads)

# test_claimer
add_executable(test_claimer unit/test_claimer.cpp)
set_target_properties(test_claimer PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_claimer")
target_link_libraries(test_claimer youdidit Threads::Threads)

# test_task_platform
add_executable(test_task_platform unit/test_task_platform.cpp)
set_target_properties(test_task_platform PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_task_platform")
target_link_libraries(test_task_platform youdidit Threads::Threads)

# test_thread_safety
add_executable(test_thread_safety unit/test_thread_safety.cpp)
set_target_properties(test_thread_safety PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_thread_safety")
target_link_libraries(test_thread_safety youdidit Threads::Threads)

# test_concurrent_execution
add_executable(test_concurrent_execution unit/test_concurrent_execution.cpp)
set_target_properties(test_concurrent_execution PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_concurrent_execution")
target_link_libraries(test_concurrent_execution youdidit Threads::Threads)

# test_finalize_idempotent
add_executable(test_finalize_idempotent unit/test_finalize_idempotent.cpp)
set_target_properties(test_finalize_idempotent PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_finalize_idempotent")
target_link_libraries(test_finalize_idempotent youdidit Threads::Threads)

# Web tests 已迁移到 `web/tests/` 子工程

# 集成测试
add_executable(integration_test_workflow integration/test_workflow.cpp)
set_target_properties(integration_test_workflow PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}integration_test_workflow")
target_link_libraries(integration_test_workflow youdidit Threads::Threads)
add_test(NAME integration_workflow COMMAND $<TARGET_FILE:integration_test_workflow>)

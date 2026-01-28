# Google Test 依赖
# 注意：需要先安装 Google Test 或使用 FetchContent
# 这里提供基本框架，实际测试将在 Phase 5 实现

# 单元测试 - 简单的独立测试（不依赖 Google Test）
# test_types
add_executable(test_types unit/test_types.cpp)
target_link_libraries(test_types youdidit_core)

# test_task
add_executable(test_task unit/test_task.cpp)
target_link_libraries(test_task youdidit_core Threads::Threads)

# test_task_builder
add_executable(test_task_builder unit/test_task_builder.cpp)
target_link_libraries(test_task_builder youdidit_core Threads::Threads)

# test_claimer
add_executable(test_claimer unit/test_claimer.cpp)
target_link_libraries(test_claimer youdidit_core Threads::Threads)

# test_task_platform
add_executable(test_task_platform unit/test_task_platform.cpp)
target_link_libraries(test_task_platform youdidit_core Threads::Threads)

# test_thread_safety
add_executable(test_thread_safety unit/test_thread_safety.cpp)
target_link_libraries(test_thread_safety youdidit_core Threads::Threads)

# test_web
add_executable(test_web unit/test_web.cpp)
target_link_libraries(test_web youdidit_core youdidit_web Threads::Threads)

# 集成测试
add_executable(integration_test_workflow integration/test_workflow.cpp)
target_link_libraries(integration_test_workflow youdidit_core Threads::Threads)
add_test(NAME integration_workflow COMMAND integration_test_workflow)

add_executable(integration_test_web_api integration/test_web_api.cpp)
target_link_libraries(integration_test_web_api youdidit_core youdidit_web Threads::Threads)
add_test(NAME integration_web_api COMMAND integration_test_web_api)

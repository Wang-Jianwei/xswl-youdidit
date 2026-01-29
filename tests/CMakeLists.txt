# Google Test 依赖
# 注意：需要先安装 Google Test 或使用 FetchContent
# 这里提供基本框架，实际测试将在 Phase 5 实现

# 单元测试 - 简单的独立测试（不依赖 Google Test）
# test_types
add_executable(test_types unit/test_types.cpp)
set_target_properties(test_types PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_types")
target_link_libraries(test_types youdidit_core)

# test_task
add_executable(test_task unit/test_task.cpp)
set_target_properties(test_task PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_task")
target_link_libraries(test_task youdidit_core Threads::Threads)

# test_task_builder
add_executable(test_task_builder unit/test_task_builder.cpp)
set_target_properties(test_task_builder PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_task_builder")
target_link_libraries(test_task_builder youdidit_core Threads::Threads)

# test_claimer
add_executable(test_claimer unit/test_claimer.cpp)
set_target_properties(test_claimer PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_claimer")
target_link_libraries(test_claimer youdidit_core Threads::Threads)

# test_task_platform
add_executable(test_task_platform unit/test_task_platform.cpp)
set_target_properties(test_task_platform PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_task_platform")
target_link_libraries(test_task_platform youdidit_core Threads::Threads)

# test_thread_safety
add_executable(test_thread_safety unit/test_thread_safety.cpp)
set_target_properties(test_thread_safety PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_thread_safety")
target_link_libraries(test_thread_safety youdidit_core Threads::Threads)

# test_web
if(NOT SKIP_WEB)
	add_executable(test_web unit/test_web.cpp)
	set_target_properties(test_web PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_web")
	target_link_libraries(test_web youdidit_core youdidit_web Threads::Threads)
endif()

# 集成测试
add_executable(integration_test_workflow integration/test_workflow.cpp)
set_target_properties(integration_test_workflow PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}integration_test_workflow")
target_link_libraries(integration_test_workflow youdidit_core Threads::Threads)
add_test(NAME integration_workflow COMMAND $<TARGET_FILE:integration_test_workflow>)

if(NOT SKIP_WEB)
	add_executable(integration_test_web_api integration/test_web_api.cpp)
	set_target_properties(integration_test_web_api PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}integration_test_web_api")
	target_link_libraries(integration_test_web_api youdidit_core youdidit_web Threads::Threads)
	add_test(NAME integration_web_api COMMAND $<TARGET_FILE:integration_test_web_api>)
endif()

# 示例程序
add_executable(example_basic_usage basic_usage.cpp)
set_target_properties(example_basic_usage PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_basic_usage")
target_link_libraries(example_basic_usage youdidit Threads::Threads)

add_executable(example_multi_claimer multi_claimer.cpp)
set_target_properties(example_multi_claimer PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_multi_claimer")
target_link_libraries(example_multi_claimer youdidit Threads::Threads)

# Web 示例已迁移到 `web/examples/` 子工程


add_executable(example_signals_usage signals_usage.cpp)
set_target_properties(example_signals_usage PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_signals_usage")
target_link_libraries(example_signals_usage youdidit Threads::Threads)

add_executable(example_perf_monitor perf_monitor.cpp)
set_target_properties(example_perf_monitor PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_perf_monitor")
target_link_libraries(example_perf_monitor youdidit Threads::Threads)

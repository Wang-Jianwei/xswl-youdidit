# 示例程序
add_executable(example_basic_usage basic_usage.cpp)
target_link_libraries(example_basic_usage youdidit_core Threads::Threads)

add_executable(example_multi_claimer multi_claimer.cpp)
target_link_libraries(example_multi_claimer youdidit_core Threads::Threads)

add_executable(example_web_monitoring web_monitoring.cpp)
target_link_libraries(example_web_monitoring youdidit_core youdidit_web Threads::Threads)

add_executable(example_web_demo web_demo.cpp)
target_link_libraries(example_web_demo youdidit_core youdidit_web Threads::Threads)

add_executable(example_signals_usage signals_usage.cpp)
target_link_libraries(example_signals_usage youdidit_core Threads::Threads)

add_executable(example_perf_monitor perf_monitor.cpp)
target_link_libraries(example_perf_monitor youdidit_core Threads::Threads)

# 示例程序
add_executable(example_basic_usage basic_usage.cpp)
set_target_properties(example_basic_usage PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_basic_usage")
target_link_libraries(example_basic_usage youdidit_core Threads::Threads)

add_executable(example_multi_claimer multi_claimer.cpp)
set_target_properties(example_multi_claimer PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_multi_claimer")
target_link_libraries(example_multi_claimer youdidit_core Threads::Threads)


if(NOT SKIP_WEB)
	add_executable(example_web_monitoring web_monitoring.cpp)
	set_target_properties(example_web_monitoring PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_web_monitoring")
	target_link_libraries(example_web_monitoring youdidit_core youdidit_web Threads::Threads)

	add_executable(example_web_demo web_demo.cpp)
	set_target_properties(example_web_demo PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_web_demo")
	target_link_libraries(example_web_demo youdidit_core youdidit_web Threads::Threads)
endif()

add_executable(example_signals_usage signals_usage.cpp)
set_target_properties(example_signals_usage PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_signals_usage")
target_link_libraries(example_signals_usage youdidit_core Threads::Threads)

add_executable(example_perf_monitor perf_monitor.cpp)
set_target_properties(example_perf_monitor PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_perf_monitor")
target_link_libraries(example_perf_monitor youdidit_core Threads::Threads)

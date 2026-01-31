add_executable(example_web_demo web_demo.cpp)
set_target_properties(example_web_demo PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}example_web_demo")
target_link_libraries(example_web_demo youdidit youdidit_web Threads::Threads)

# Web tests
add_executable(test_web unit/test_web.cpp)
set_target_properties(test_web PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}test_web")
target_link_libraries(test_web youdidit youdidit_web Threads::Threads)

add_executable(integration_test_web_api integration/test_web_api.cpp)
set_target_properties(integration_test_web_api PROPERTIES OUTPUT_NAME "${EASY_EXECUTABLE_PREFIX}integration_test_web_api")
target_link_libraries(integration_test_web_api youdidit youdidit_web Threads::Threads)
add_test(NAME integration_web_api COMMAND $<TARGET_FILE:integration_test_web_api>)

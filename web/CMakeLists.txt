# Web 子工程
file(GLOB_RECURSE WEB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(WEB_SOURCES)
    add_library(youdidit_web ${WEB_SOURCES})
    target_include_directories(youdidit_web PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    # Ensure Windows 10+ for cpp-httplib when building on Windows
    if(WIN32)
        target_compile_definitions(youdidit_web PRIVATE _WIN32_WINNT=0x0A00)
    endif()
    if(WIN32)
        target_link_libraries(youdidit_web PUBLIC youdidit Threads::Threads ws2_32 iphlpapi)
    else()
        target_link_libraries(youdidit_web PUBLIC youdidit Threads::Threads)
    endif()
    target_link_libraries(youdidit_web PRIVATE xswl_youdidit_deps)
endif()

# 可选构建：示例与测试
# On Windows toolchains we default to OFF due to platform-specific link issues
if(WIN32)
    option(BUILD_WEB_EXAMPLES "Build web examples" OFF)
    option(BUILD_WEB_TESTS "Build web tests" OFF)
else()
    option(BUILD_WEB_EXAMPLES "Build web examples" ON)
    option(BUILD_WEB_TESTS "Build web tests" ON)
endif()

if(BUILD_WEB_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_WEB_TESTS)
    add_subdirectory(tests)
endif()

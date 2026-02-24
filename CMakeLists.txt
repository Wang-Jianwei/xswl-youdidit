cmake_minimum_required(VERSION 3.10)
project(xswl-youdidit VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 第三方依赖
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/third_party/tl_optional/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/tl_expected/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/xswl_signals/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann)

# 线程库（跨平台）
find_package(Threads REQUIRED)

# 可配置的可执行文件前缀（将写入构建目录的 easy_executable_prefix.txt，供脚本读取）
set(EASY_EXECUTABLE_PREFIX "easy-" CACHE STRING "Prefix added to executable output names (set empty to disable)")
# 将前缀写入到构建目录（每次 configure 时更新），脚本会读取此文件来找到正确的可执行文件名
file(WRITE "${CMAKE_BINARY_DIR}/executable_prefix.txt" "${EASY_EXECUTABLE_PREFIX}")


# 项目头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加子目录
add_subdirectory(src)

# Web 子工程（可选）
option(BUILD_WEB "Build web module" ON)
if(BUILD_WEB)
    add_subdirectory(web)
endif()

# 测试
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 示例
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 安装
install(TARGETS youdidit
    EXPORT xswl-youdidit-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/xswl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY third_party/tl_optional/include/tl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY third_party/tl_expected/include/tl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY third_party/xswl_signals/include/xswl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT xswl-youdidit-targets
    FILE xswl-youdidit-targets.cmake
    NAMESPACE xswl::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xswl-youdidit
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/xswl-youdidit-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/xswl-youdidit-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xswl-youdidit
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/xswl-youdidit-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/xswl-youdidit-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/xswl-youdidit-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xswl-youdidit
)

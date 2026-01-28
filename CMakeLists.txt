cmake_minimum_required(VERSION 3.10)
project(xswl-youdidit VERSION 1.0.0 LANGUAGES CXX)

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

# 项目头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加子目录
add_subdirectory(src)

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
install(DIRECTORY include/xswl DESTINATION include)

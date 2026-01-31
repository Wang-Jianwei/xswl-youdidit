# 核心源文件
file(GLOB_RECURSE CORE_SOURCES core/*.cpp)

# 核心库（命名为 youdidit）
add_library(youdidit ${CORE_SOURCES})

target_include_directories(youdidit PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
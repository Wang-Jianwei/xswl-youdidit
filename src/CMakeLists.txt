# 核心源文件
file(GLOB_RECURSE CORE_SOURCES core/*.cpp)

# 核心库（已重命名为 youdidit）
add_library(youdidit ${CORE_SOURCES})
# 保留旧名称别名以兼容现有内部引用
add_library(youdidit_core ALIAS youdidit)
target_include_directories(youdidit PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 注意：Web 模块已迁移到仓库根目录的 `web/` 子工程（`web/CMakeLists.txt`）。
# 若要在本仓库构建 Web，请在根 CMake 调用时启用 BUILD_WEB 选项。

# 核心源文件
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS core/*.cpp)

# 核心库（命名为 youdidit）
add_library(youdidit ${CORE_SOURCES})
add_library(xswl::youdidit ALIAS youdidit)

target_compile_features(youdidit PUBLIC cxx_std_11)

target_include_directories(youdidit PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(youdidit PUBLIC Threads::Threads $<BUILD_INTERFACE:xswl_youdidit_deps>)
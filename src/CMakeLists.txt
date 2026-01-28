# 核心源文件
file(GLOB_RECURSE CORE_SOURCES core/*.cpp)

# 核心库
add_library(youdidit_core ${CORE_SOURCES})
target_include_directories(youdidit_core PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
)

# Web 模块（可选）
option(BUILD_WEB_MODULE "Build web monitoring module" ON)
if(BUILD_WEB_MODULE)
    file(GLOB_RECURSE WEB_SOURCES web/*.cpp)
    if(WEB_SOURCES)
        add_library(youdidit_web ${WEB_SOURCES})
        target_link_libraries(youdidit_web youdidit_core pthread)
        target_include_directories(youdidit_web PUBLIC 
            ${CMAKE_SOURCE_DIR}/include
        )
    endif()
endif()

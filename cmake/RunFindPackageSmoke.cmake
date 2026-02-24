if(NOT DEFINED ROOT_BUILD_DIR)
    message(FATAL_ERROR "ROOT_BUILD_DIR is required")
endif()
if(NOT DEFINED SMOKE_SOURCE_DIR)
    message(FATAL_ERROR "SMOKE_SOURCE_DIR is required")
endif()
if(NOT DEFINED SMOKE_BUILD_DIR)
    message(FATAL_ERROR "SMOKE_BUILD_DIR is required")
endif()
if(NOT DEFINED SMOKE_INSTALL_PREFIX)
    message(FATAL_ERROR "SMOKE_INSTALL_PREFIX is required")
endif()

if(EXISTS "${SMOKE_BUILD_DIR}")
    file(REMOVE_RECURSE "${SMOKE_BUILD_DIR}")
endif()
if(EXISTS "${SMOKE_INSTALL_PREFIX}")
    file(REMOVE_RECURSE "${SMOKE_INSTALL_PREFIX}")
endif()

set(_install_cmd "${CMAKE_COMMAND}" --install "${ROOT_BUILD_DIR}" --prefix "${SMOKE_INSTALL_PREFIX}")
if(DEFINED SMOKE_CONFIG AND NOT "${SMOKE_CONFIG}" STREQUAL "")
    list(APPEND _install_cmd --config "${SMOKE_CONFIG}")
endif()

execute_process(
    COMMAND ${_install_cmd}
    RESULT_VARIABLE _install_result
)
if(NOT _install_result EQUAL 0)
    message(FATAL_ERROR "find_package smoke: install failed with exit code ${_install_result}")
endif()

set(_configure_cmd "${CMAKE_COMMAND}" -S "${SMOKE_SOURCE_DIR}" -B "${SMOKE_BUILD_DIR}" "-DCMAKE_PREFIX_PATH=${SMOKE_INSTALL_PREFIX}")
if(DEFINED SMOKE_GENERATOR AND NOT "${SMOKE_GENERATOR}" STREQUAL "")
    list(APPEND _configure_cmd -G "${SMOKE_GENERATOR}")
endif()
if(DEFINED SMOKE_MAKE_PROGRAM AND NOT "${SMOKE_MAKE_PROGRAM}" STREQUAL "")
    if(EXISTS "${SMOKE_MAKE_PROGRAM}")
        list(APPEND _configure_cmd "-DCMAKE_MAKE_PROGRAM=${SMOKE_MAKE_PROGRAM}")
    else()
        message(WARNING "SMOKE_MAKE_PROGRAM does not exist, ignore: ${SMOKE_MAKE_PROGRAM}")
    endif()
endif()
if(DEFINED SMOKE_BUILD_TYPE AND NOT "${SMOKE_BUILD_TYPE}" STREQUAL "")
    list(APPEND _configure_cmd "-DCMAKE_BUILD_TYPE=${SMOKE_BUILD_TYPE}")
endif()

execute_process(
    COMMAND ${_configure_cmd}
    RESULT_VARIABLE _configure_result
)
if(NOT _configure_result EQUAL 0)
    message(FATAL_ERROR "find_package smoke: configure consumer failed with exit code ${_configure_result}")
endif()

set(_build_cmd "${CMAKE_COMMAND}" --build "${SMOKE_BUILD_DIR}")
if(DEFINED SMOKE_CONFIG AND NOT "${SMOKE_CONFIG}" STREQUAL "")
    list(APPEND _build_cmd --config "${SMOKE_CONFIG}")
endif()

execute_process(
    COMMAND ${_build_cmd}
    RESULT_VARIABLE _build_result
)
if(NOT _build_result EQUAL 0)
    message(FATAL_ERROR "find_package smoke: build consumer failed with exit code ${_build_result}")
endif()

message(STATUS "find_package smoke test passed")

cmake_minimum_required(VERSION 3.10)
project(xswl_youdidit_consumer_smoke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(xswl-youdidit REQUIRED CONFIG)

add_executable(consumer_smoke main.cpp)
target_link_libraries(consumer_smoke PRIVATE xswl::youdidit)

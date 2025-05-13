cmake_minimum_required(VERSION 3.10)
project(Yftp)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(Yftp ${SOURCES})
target_include_directories(Yftp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(Yftp
    PRIVATE
    boost_system
    pthread
    spdlog
    fmt
    jsoncpp
    yaml-cpp
    tinyxml2
)


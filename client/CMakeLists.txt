cmake_minimum_required(VERSION 3.10)
project(YftpClient)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(yftp_client ${SOURCES})
target_include_directories(yftp_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(yftp_client
    PRIVATE
    pthread
    boost_system
    boost_program_options
    jsoncpp
    z
)


cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(
    PORTAL301_ZED_Application
    VERSION 1.0
)

add_executable(program main.cpp)
add_subdirectory(lib)


target_include_directories(
    program
        PUBLIC
        ${CMAKE_SOURCE_DIR}/includes
)


target_link_libraries(program 
        myZed
        portalrtc
        portalcomm
    ) 
target_compile_options(program PUBLIC -Wall -Werror)

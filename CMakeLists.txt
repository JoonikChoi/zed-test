cmake_minimum_required(VERSION 3.22)

project(
    PORTAL301_ZED_Application
    VERSION 1.0
)

# Set the C++ standard (change if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_GENERATOR MATCHES "Visual Studio")

    # Do Visual Studio specific stuff
	message("build for Visual Studio, MSVC and VCPKG")

    # Include vcpkg toolchain file
    # set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg target triplet")
    # if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    #    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    #        CACHE STRING "")

    # Find required packages
    # find_package(your_required_package1 REQUIRED)
    # find_package(your_required_package2 REQUIRED)

    # Set include directories
    include_directories(includes)

    # Add subdirectories for libraries and main executable
    add_subdirectory(lib)

    # Add the main executable
    add_executable(PORTAL301_ZED_Application main.cpp)

    # Link against the required libraries
    # target_link_libraries(PORTAL301_ZED_Application PRIVATE your_required_package1::your_required_package1 your_required_package2::your_required_package2)

    # Link against the libraries you built in the lib directory
    target_link_libraries(PORTAL301_ZED_Application PRIVATE portalComm portalRTC poGst)


    else()
    message("build for gcc/g++ and make")
    find_package(OpenCV REQUIRED)
    add_executable(program main.cpp)
    add_subdirectory(lib)


    target_include_directories(
        program
            PUBLIC
            ${CMAKE_SOURCE_DIR}/includes
            ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(program 
            # myZed
            portalrtc
            portalcomm
            ${OpenCV_LIBS}
        ) 
    target_compile_options(program PUBLIC -Wall -Werror)
endif()

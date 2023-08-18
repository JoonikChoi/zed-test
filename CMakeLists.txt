cmake_minimum_required(VERSION 3.22)

project(
    PORTAL301_ZED_Application
    VERSION 1.0
)

# Set the C++ standard (change if needed)
set(CMAKE_CXX_STANDARD 17)


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
    include_directories(
        include
        "C:/Program Files (x86)/ZED SDK/include"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.7/include"
    )

    # Add subdirectories for libraries and main executable
    add_subdirectory(lib)

    # Add the main executable
    add_executable(PORTAL301_ZED_Application main.cpp)

    # Link against the required libraries
    # target_link_libraries(PORTAL301_ZED_Application PRIVATE your_required_package1::your_required_package1 your_required_package2::your_required_package2)

    # Link against the libraries you built in the lib directory
    target_link_libraries(PORTAL301_ZED_Application PRIVATE portalZed portalComm portalRTC)

    # Set linker options
    target_link_directories(PORTAL301_ZED_Application PRIVATE
        "C:/Program Files (x86)/ZED SDK/lib"
        "C:/Program Files (x86)/ZED SDK/lib/Release"
    )

    # Link additional dependencies
    target_link_libraries(PORTAL301_ZED_Application PRIVATE
        "C:/Program Files (x86)/ZED SDK/lib/sl_zed64.lib"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.7/lib/x64/cuda.lib"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.7/lib/x64/cudart.lib"
    )
else()
	message("build for gcc/g++ and make")
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
endif()

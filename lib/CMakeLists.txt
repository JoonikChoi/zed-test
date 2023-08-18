if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # CMakeLists.txt inside the lib directory
    add_library(portalZed portalZed.cpp)
    add_library(portalComm portalComm.cpp)
    add_library(portalRTC portalRTC.cpp)

    # Set include directories for the libraries
    target_include_directories(portalZed PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(portalComm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(portalRTC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
else()
    cmake_minimum_required(VERSION 3.22)

    include(FetchContent)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_BUILD_TYPE Release)

    ## build webRTC part ###

    FetchContent_Declare(json
            GIT_REPOSITORY https://github.com/nlohmann/json
            GIT_TAG v3.10.5
    )
    FetchContent_MakeAvailable(json)
    find_package(LibDataChannel REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(sioclient REQUIRED)
    find_package(Threads REQUIRED)

    add_library(portalrtc portalRTC.cpp)
    target_include_directories(portalrtc PUBLIC ${CMAKE_SOURCE_DIR}/includes)

    target_compile_options(portalrtc PUBLIC -Wall -Werror -fPIC)
    target_link_libraries(portalrtc
        PUBLIC
            LibDataChannel::LibDataChannel
            nlohmann_json::nlohmann_json
    )

    add_library(portalcomm portalComm.cpp)
    target_include_directories(portalcomm PUBLIC ${CMAKE_SOURCE_DIR}/includes)
    target_compile_options(portalcomm PUBLIC -Wall -Werror -fPIC)

    target_link_libraries(portalcomm
        PUBLIC
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            sioclient::sioclient_tls
            Threads::Threads
    )

    ###############################
    ### build operate zed part ###

    option(LINK_SHARED_ZED "Link with the ZED SDK shared executable" ON)
    if (NOT LINK_SHARED_ZED AND MSVC)
        message(FATAL_ERROR "LINK_SHARED_ZED OFF : ZED SDK static libraries not available on Windows")
    endif()


    find_package(ZED 3 REQUIRED)
    find_package(CUDA ${ZED_CUDA_VERSION} REQUIRED)
    find_package(OpenCV REQUIRED)

    # include_directories(${CUDA_INCLUDE_DIRS})
    # include_directories(${ZED_INCLUDE_DIRS})
    # include_directories(${OpenCV_INCLUDE_DIRS})

    # link_directories(${ZED_LIBRARY_DIR})
    # link_directories(${CUDA_LIBRARY_DIRS})
    # link_directories(${OpenCV_LIBRARY_DIRS})

    add_library(myZed portalZed.cpp)
    target_include_directories(myZed PUBLIC ${CMAKE_SOURCE_DIR}/includes)
    target_compile_options(myZed PUBLIC -Wall -Werror)

    if (LINK_SHARED_ZED)
        message("LINK_SHARED_ZED ON")
        SET(ZED_LIBS ${ZED_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY})
    else()
        message("LINK_SHARED_ZED OFF")
        SET(ZED_LIBS ${ZED_STATIC_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_LIBRARY})
    endif()

    target_include_directories(myZed
        PUBLIC
            ${CMAKE_SOURCE_DIR}/includes
            ${ZED_INCLUDE_DIRS}
            ${CUDA_INCLUDE_DIRS}
            ${OpenCV_INCLUDE_DIRS}
    )

    target_link_directories(myZed
        PUBLIC
            ${ZED_LIBRARY_DIR}
            ${OpenCV_LIBRARY_DIRS}
    )

    target_link_libraries(myZed
        PUBLIC
            ${ZED_LIBS}
            ${OpenCV_LIBS}
            nlohmann_json::nlohmann_json
        )

endif()
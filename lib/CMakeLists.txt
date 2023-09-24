if (CMAKE_GENERATOR MATCHES "Visual Studio")
    # CMakeLists.txt inside the lib directory
    add_library(portalComm portalComm.cpp)
    add_library(portalRTC portalRTC.cpp)

    # Set include directories for the libraries
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
    find_package(CURL REQUIRED)

    add_library(portalrtc portalRTC.cpp)
    target_include_directories(portalrtc PUBLIC ${CMAKE_SOURCE_DIR}/includes)

    target_compile_options(portalrtc PUBLIC -Wall -Werror -fPIC)
    target_link_libraries(portalrtc
        PUBLIC
            LibDataChannel::LibDataChannel
            nlohmann_json::nlohmann_json
    )

    #######################################

    add_library(portalcomm portalComm.cpp)
    target_include_directories(portalcomm PUBLIC ${CMAKE_SOURCE_DIR}/includes)
    target_compile_options(portalcomm PUBLIC -Wall -Werror -fPIC)

    target_include_directories(portalcomm
    PUBLIC
        ${CURL_INCLUDE_DIRS}
    )

    target_link_libraries(portalcomm
        PUBLIC
            nlohmann_json::nlohmann_json
            OpenSSL::SSL
            sioclient::sioclient_tls
            Threads::Threads
            ${CURL_LIBRARIES}
    )

    ###############################

endif()
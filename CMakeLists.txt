cmake_minimum_required(VERSION 3.24)

project(ChargingController LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(MAKE_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/config
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config/control_parameters.json
        ${CMAKE_CURRENT_BINARY_DIR}/config/control_parameters.json
        COPYONLY
)

include(FetchContent)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)

add_executable(ChargingController
        src/main.cpp
        src/ChargingController.cpp
        src/CurrentController.cpp

        hal/hal.c

        protocol/protocol.c
)

target_include_directories(ChargingController PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/hal
        ${CMAKE_CURRENT_SOURCE_DIR}/protocol
)

target_link_libraries(
        ChargingController
        PRIVATE
        nlohmann_json::nlohmann_json
)
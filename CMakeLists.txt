cmake_minimum_required(VERSION 3.16)

project(labwork9_linuxen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_IGNORE_PREFIX_PATH
    "/mnt/c/msys64"
    "/mnt/c/msys64/ucrt64"
    "/mnt/c/msys64/mingw64"
)

list(APPEND CMAKE_SYSTEM_IGNORE_PREFIX_PATH
    "/mnt/c/msys64"
    "/mnt/c/msys64/ucrt64"
    "/mnt/c/msys64/mingw64"
)

list(APPEND CMAKE_IGNORE_PATH
    "/mnt/c/msys64/ucrt64/lib/cmake/CURL"
    "/mnt/c/msys64/mingw64/lib/cmake/CURL"
)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG OFF)

unset(CURL_DIR CACHE)
unset(CURL_INCLUDE_DIR CACHE)
unset(CURL_INCLUDE_DIRS CACHE)
unset(CURL_LIBRARY CACHE)
unset(CURL_LIBRARIES CACHE)

set(CURL_NO_CURL_CMAKE ON CACHE BOOL "" FORCE)

include(FetchContent)

set(JSON_BuildTests OFF CACHE INTERNAL "")

set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)

find_package(CURL REQUIRED MODULE)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.11.2
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(cpr json googletest)

add_library(task_scheduler INTERFACE)

target_include_directories(task_scheduler INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(task_scheduler_app
    bin/main.cpp
)

target_link_libraries(task_scheduler_app PRIVATE
    task_scheduler
)

add_executable(recommendation_app
    main/main.cpp
    src/getLocation.cpp
    src/getWether.cpp
    src/getPlaces.cpp
)

target_link_libraries(recommendation_app PRIVATE
    task_scheduler
    cpr::cpr
    nlohmann_json::nlohmann_json
)

enable_testing()

add_executable(labwork_tests
    tests/task_sheduller_test.cpp
    tests/task_12_test.cpp

    src/getLocation.cpp
    src/getWether.cpp
    src/getPlaces.cpp
)

target_link_libraries(labwork_tests PRIVATE
    task_scheduler
    cpr::cpr
    nlohmann_json::nlohmann_json
    GTest::gmock_main
)

include(GoogleTest)
gtest_discover_tests(labwork_tests)

add_custom_target(run_task_scheduler
    COMMAND task_scheduler_app
    DEPENDS task_scheduler_app
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(run_recommendation
    COMMAND recommendation_app
    DEPENDS recommendation_app
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(run_tests
    COMMAND labwork_tests
    DEPENDS labwork_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
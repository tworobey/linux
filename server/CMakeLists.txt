cmake_minimum_required(VERSION 3.16)
project(game_server CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/model.cpp
    src/boost_json.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/loot_generator.cpp
    src/collision_detector.cpp
)

target_include_directories(game_server PRIVATE
    CONAN_PKG::boost
)

target_link_libraries(game_server PRIVATE
    Threads::Threads
    CONAN_PKG::boost
)

target_compile_definitions(game_server PRIVATE
    BOOST_LOG_USE_NATIVE_SYSLOG
)


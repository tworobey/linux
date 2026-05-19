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

# =========================
# Общая библиотека
# =========================
add_library(game_model STATIC
    src/model.cpp
    src/boost_json.cpp
    src/loot_generator.cpp
    src/collision_detector.cpp
    src/logging.cpp
)

target_include_directories(game_model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_model PUBLIC
    Threads::Threads
    CONAN_PKG::boost
)

# =========================
# Основной сервер
# =========================
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/json_loader.cpp
    src/request_handler.cpp
)

target_include_directories(game_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_server PRIVATE
    game_model
)

target_compile_definitions(game_server PRIVATE
    BOOST_LOG_USE_NATIVE_SYSLOG
)

# =========================
# Тесты
# =========================
add_executable(game_server_tests
    tests/collision-detector-tests.cpp
    tests/loot_generator_tests.cpp
    tests/model-tests.cpp

    src/model.cpp
    src/boost_json.cpp
    src/loot_generator.cpp
    src/collision_detector.cpp
    src/logging.cpp
)

target_include_directories(game_server_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(game_server_tests PRIVATE
    Threads::Threads
    CONAN_PKG::boost
    CONAN_PKG::catch2
)

enable_testing()

add_test(NAME game_server_tests COMMAND game_server_tests)

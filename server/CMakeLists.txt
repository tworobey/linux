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

# Статическая библиотека с общим кодом
add_library(game_model STATIC
    src/model.cpp
    src/boost_json.cpp
    src/loot_generator.cpp
    src/collision_detector.cpp
    src/logging.cpp
)

target_include_directories(game_model PUBLIC
    CONAN_PKG::boost
    src
)

target_link_libraries(game_model PUBLIC
    Threads::Threads
    CONAN_PKG::boost
)

# Основной исполняемый файл
add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/json_loader.cpp
    src/request_handler.cpp
)

target_link_libraries(game_server PRIVATE
    game_model
    CONAN_PKG::libpqxx
)

target_compile_definitions(game_server PRIVATE
    BOOST_LOG_USE_NATIVE_SYSLOG
)

# Тесты
add_executable(game_server_tests
    tests/collision-detector-tests.cpp
    tests/loot_generator_tests.cpp
    tests/model-tests.cpp
    src/model.cpp
    src/loot_generator.cpp
    src/collision_detector.cpp
)

target_include_directories(game_server_tests PRIVATE
    src
    CONAN_PKG::boost
)

target_link_libraries(game_server_tests PRIVATE
    CONAN_PKG::catch2
    Threads::Threads
)


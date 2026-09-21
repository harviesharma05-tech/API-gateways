cmake_minimum_required(VERSION 3.10)
project(api_gateway_prototype CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Threads::Threads resolves correctly on Windows (no -pthread needed),
# Linux and macOS (-pthread added automatically) — no manual OS checks.
find_package(Threads REQUIRED)

add_executable(gateway src/main.cpp)
target_include_directories(gateway PRIVATE include)
target_link_libraries(gateway PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(gateway PRIVATE /W4)
else()
    target_compile_options(gateway PRIVATE -Wall -Wextra)
endif()

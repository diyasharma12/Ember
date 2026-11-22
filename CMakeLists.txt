cmake_minimum_required(VERSION 3.16)
project(bedis CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(bedis
    src/main.cpp
    src/config/config.cpp
    src/core/obj.cpp
    src/core/resp.cpp
    src/core/store.cpp
    src/core/stats.cpp
    src/core/validation.cpp
    src/core/evictionpool.cpp
    src/core/eviction.cpp
    src/core/expire.cpp
    src/core/aof.cpp
    src/core/comm.cpp
    src/core/eval.cpp
    src/server/tcp.cpp
)

target_include_directories(bedis PRIVATE src)
target_compile_options(bedis PRIVATE -Wall -Wextra)
target_link_libraries(bedis PRIVATE pthread)

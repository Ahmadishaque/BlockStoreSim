cmake_minimum_required(VERSION 3.16)
project(BlockStoreSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(blockstore
    src/block_store.cpp
)

target_include_directories(blockstore PUBLIC src)

target_compile_options(blockstore PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(blockstoresim
    src/main.cpp
)

target_link_libraries(blockstoresim PRIVATE blockstore)

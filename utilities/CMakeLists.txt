cmake_minimum_required(VERSION 3.16)

project(utilities LANGUAGES CXX)

add_library(utilities INTERFACE)

target_include_directories(utilities
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
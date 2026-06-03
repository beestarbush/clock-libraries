cmake_minimum_required(VERSION 3.16)

project(configuration LANGUAGES CXX)

add_library(configuration INTERFACE)

target_include_directories(configuration
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
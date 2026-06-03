cmake_minimum_required(VERSION 3.16)

project(communication LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Test)

add_library(communication STATIC
    src/services/websocket/Types.h
    src/services/websocket/Frame.h
    src/services/websocket/Frame.cpp
    src/services/websocket/Service.h
    src/services/websocket/Service.cpp
    src/services/websocket/SubscriptionHelper.h
    src/services/websocket/SubscriptionHelper.cpp
    src/services/configuration/DeviceConfiguration.h
    src/services/configuration/DeviceConfiguration.cpp
    src/services/protocol/MethodCatalog.h
    src/services/protocol/MethodCatalog.cpp
)

add_library(clock_common::communication ALIAS communication)

target_link_libraries(communication
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::WebSockets
)

target_include_directories(communication
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_options(communication
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

enable_testing()

add_executable(communication_unit_tests
    tests/CommunicationUnitTests.cpp
)

target_link_libraries(communication_unit_tests
    PRIVATE
        communication
        Qt6::Core
        Qt6::Test
)

add_test(NAME communication_unit_tests COMMAND communication_unit_tests)

add_executable(communication_device_configuration_unit_tests
    tests/DeviceConfigurationUnitTests.cpp
)

target_link_libraries(communication_device_configuration_unit_tests
    PRIVATE
        communication
        Qt6::Core
        Qt6::Test
)

add_test(NAME communication_device_configuration_unit_tests COMMAND communication_device_configuration_unit_tests)

install(TARGETS communication
    EXPORT ClockCommonLibrariesTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
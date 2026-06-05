cmake_minimum_required(VERSION 3.16)

project(clock-common-libraries LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

enable_testing()

if (CMAKE_CROSSCOMPILING OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|arm)")
    set(PLATFORM "target")
    add_definitions(-DPLATFORM_IS_TARGET)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM "host")
    add_definitions(-DPLATFORM_IS_HOST)
endif()

add_subdirectory(communication)
add_subdirectory(utilities)
add_subdirectory(configuration)

if (CMAKE_CROSSCOMPILING OR CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|arm)")
    add_definitions(-DPLATFORM_IS_TARGET)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_IS_HOST)
endif()

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/ClockCommonLibrariesConfigVersion.cmake
	VERSION 0.1.0
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/ClockCommonLibrariesConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/ClockCommonLibrariesConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ClockCommonLibraries
)

install(EXPORT ClockCommonLibrariesTargets
	FILE ClockCommonLibrariesTargets.cmake
	NAMESPACE clock_common::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ClockCommonLibraries
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/ClockCommonLibrariesConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/ClockCommonLibrariesConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ClockCommonLibraries
)
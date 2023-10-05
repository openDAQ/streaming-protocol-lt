cmake_minimum_required(VERSION 3.24)

if (NOT CMAKE_MESSAGE_CONTEXT)
    set(CMAKE_MESSAGE_CONTEXT bbstreaming)
    set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")
endif()

project(streaming_protocol VERSION 0.10.5 LANGUAGES CXX)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

get_filename_component(ROOT_DIR ${CMAKE_SOURCE_DIR} REALPATH)
if (NOT ${PROJECT_SOURCE_DIR} STREQUAL ${ROOT_DIR})
    set(BUILDING_AS_SUBMODULE ON)
    message(STATUS "Building as submodule")
else()
    message(STATUS "Building standalone")
    set(CMAKE_FOLDER "${CMAKE_FOLDER}/bbstreaming")
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".CMakePredefinedTargets")
endif()

option(STREAMING_PROTOCOL_POST_BUILD_UNITTEST "Automatically run unit-tests as a post build step" OFF)
option(STREAMING_PROTOCOL_TOOLS "Enable building some tools that use the streaming_protocol library" OFF)
option(ENABLE_ASAN "Build with address sanitizer" OFF)

set(OPENDAQ_REPO_PREFIX "git@github.com:blueberrydaq" CACHE STRING "Set this if using a repository mirror")
message(STATUS "BB repository prefix: ${OPENDAQ_REPO_PREFIX}")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake
)

set(FETCHCONTENT_EXTERNALS_DIR ${ROOT_DIR}/build/__external CACHE PATH "FetchContent folder prefix")

include(StreamingUtils)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (WIN32)
    set(MIN_SUPPORTED_WINDOWS 0x0601)
    add_compile_definitions(WIN32_LEAN_AND_MEAN
                            NOMINMAX
                            _WIN32_WINNT=${MIN_SUPPORTED_WINDOWS} # Windows 7 Compat
    )

    message(STATUS "Setting minimum supported API to Windows 7 (${MIN_SUPPORTED_WINDOWS})")
endif()

if (NOT COMMAND set_mode)
    message(STATUS "Including Modern.cmake")
    include(Modern)
else()
    set_mode(MODERN)
endif()

add_subdirectory(external)
add_subdirectory(src)

if(STREAMING_PROTOCOL_TOOLS)
    add_subdirectory(tool)
endif(STREAMING_PROTOCOL_TOOLS)

if(STREAMING_PROTOCOL_POST_BUILD_UNITTEST)
    enable_testing()
    add_subdirectory(test)
endif(STREAMING_PROTOCOL_POST_BUILD_UNITTEST)

# install the library and generate export set (note that no
# targets file is generated or installed in this step)
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

# install the public headers
target_include_directories(${PROJECT_NAME} PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# install a targets file for the generated export set
install(
    EXPORT ${PROJECT_NAME}
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE daq::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${${PROJECT_NAME}_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

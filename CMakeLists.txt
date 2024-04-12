cmake_minimum_required(VERSION 3.24)

if (NOT CMAKE_MESSAGE_CONTEXT)
    set(CMAKE_MESSAGE_CONTEXT openDaqStreaming)
    set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")
endif()

project(streaming_protocol_complete VERSION 0.10.17 LANGUAGES CXX)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

get_filename_component(ROOT_DIR ${CMAKE_SOURCE_DIR} REALPATH)
if (NOT ${PROJECT_SOURCE_DIR} STREQUAL ${ROOT_DIR})
    set(BUILDING_AS_SUBMODULE ON)
    message(STATUS "Building as submodule")
else()
    message(STATUS "Building standalone")
    set(CMAKE_FOLDER "${CMAKE_FOLDER}/openDaqStreaming")
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".CMakePredefinedTargets")
endif()

option(STREAMING_PROTOCOL_SPECIFICATION "Enable building the specification" OFF)
option(STREAMING_PROTOCOL_LIB "Enable building the library implementing the protocol" ON)
option(STREAMING_PROTOCOL_TOOLS "Enable building some tools that use the streaming_protocol library" ON)
option(STREAMING_PROTOCOL_POST_BUILD_UNITTEST "Automatically run unit-tests as a post build step" OFF)

set(OPENDAQ_REPO_PREFIX "https://github.com/openDAQ" CACHE STRING "Set this if using a repository mirror")
message(STATUS "openDAQ repository prefix: ${OPENDAQ_REPO_PREFIX}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(STREAMING_PROTOCOL_SPECIFICATION)
    add_subdirectory(docs)
endif(STREAMING_PROTOCOL_SPECIFICATION)

if(STREAMING_PROTOCOL_LIB)
    add_subdirectory(lib)
endif(STREAMING_PROTOCOL_LIB)


if(STREAMING_PROTOCOL_TOOLS)
    add_subdirectory(tool)
endif(STREAMING_PROTOCOL_TOOLS)

if(STREAMING_PROTOCOL_POST_BUILD_UNITTEST)
    enable_testing()
    add_subdirectory(test)
endif(STREAMING_PROTOCOL_POST_BUILD_UNITTEST)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.24)

if (NOT CMAKE_MESSAGE_CONTEXT)
    set(CMAKE_MESSAGE_CONTEXT openDaqStreaming)
    set(CMAKE_MESSAGE_CONTEXT_SHOW ON CACHE BOOL "Show CMake message context")
endif()

project(streaming_protocol_complete VERSION 1.2.8 LANGUAGES CXX)

add_subdirectory(cmake)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

opendaq_get_cmake_mode(_opendaq_current_mode)
if (NOT _opendaq_current_mode)
    opendaq_set_cmake_mode(MODERN)
endif()

get_filename_component(ROOT_DIR ${CMAKE_SOURCE_DIR} REALPATH)

if (PROJECT_IS_TOP_LEVEL)
    message(STATUS "Building standalone")
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".CMakePredefinedTargets")
else()
    message(STATUS "Building as submodule")
endif()

option(STREAMING_PROTOCOL_SPECIFICATION "Enable building the specification" OFF)
option(STREAMING_PROTOCOL_LIB "Enable building the library implementing the protocol" ON)
option(STREAMING_PROTOCOL_TOOLS "Enable building some tools that use the streaming_protocol library" ON)
option(STREAMING_PROTOCOL_POST_BUILD_UNITTEST "Automatically run unit-tests as a post build step" OFF)

set(OPENDAQ_REPO_PREFIX "https://github.com/openDAQ" CACHE STRING "Set this if using a repository mirror")
message(STATUS "openDAQ repository prefix: ${OPENDAQ_REPO_PREFIX}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++17>)
endif(MSVC)

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


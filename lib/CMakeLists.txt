cmake_minimum_required(VERSION 3.24)

project(streaming_protocol VERSION "${streaming_protocol_complete_VERSION_MAJOR}.${streaming_protocol_complete_VERSION_MINOR}.${streaming_protocol_complete_VERSION_PATCH}" LANGUAGES CXX)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()


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


# interface headers
set(STREAMING_PROTOCOL_STREAMING_INTERFACE_HEADERS
    # common
    common.hpp
    Defines.h
    jsonrpc_defines.hpp
    Logging.hpp
    TimeResolution.hpp
    Types.h
    Unit.hpp

    # consumer
    MetaInformation.hpp
    ProtocolHandler.hpp
    SignalContainer.hpp
    StreamMeta.hpp
    SubscribedSignal.hpp

    # producer
    AsynchronousSignal.hpp
    BaseSignal.hpp
    BaseDomainSignal.hpp
    BaseValueSignal.hpp
    BaseSynchronousSignal.hpp
    ExplicitTimeSignal.hpp
    LinearTimeSignal.hpp
    ControlServer.hpp
    ProducerSession.hpp
    Server.hpp
    StreamWriter.h
    SynchronousSignal.hpp
    BaseConstantSignal.hpp
    ConstantSignal.hpp
)


prepend_include(${PROJECT_NAME} STREAMING_PROTOCOL_STREAMING_INTERFACE_HEADERS)

set(STREAMING_PROTOCOL_STREAMING_LIB_SOURCES
    ${STREAMING_PROTOCOL_STREAMING_INTERFACE_HEADERS}
    # common
    Logging.cpp
    TimeResolution.cpp
    Types.cpp
    Unit.cpp
    utils/strings.hpp

    # consumer
    Controller.cpp
    Controller.hpp
    HttpPost.cpp
    HttpPost.hpp
    MetaInformation.cpp
    ProtocolHandler.cpp
    SignalContainer.cpp
    StreamMeta.cpp
    SubscribedSignal.cpp

    # producer
    AsynchronousSignal.cpp
    BaseSignal.cpp
    BaseDomainSignal.cpp
    BaseValueSignal.cpp
    BaseSynchronousSignal.cpp
    ExplicitTimeSignal.cpp
    LinearTimeSignal.cpp
    ControlServer.cpp
    ProducerSession.cpp
    Server.cpp
    StreamWriter.cpp
    SynchronousSignal.cpp
    BaseConstantSignal.cpp
    ConstantSignal.cpp
)

add_library(${PROJECT_NAME} ${STREAMING_PROTOCOL_STREAMING_LIB_SOURCES})
add_library(daq::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# required if this library will ever be linked into a shared object
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json
                                             daq::stream
                                             spdlog::spdlog
                                             fmt::fmt
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  CXX_STANDARD_REQUIRED ON
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
    BOOST_ALL_NO_LIB
)


if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
    if ("${MSVC_VERSION}" LESS 1920 AND "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
        target_compile_options(${PROJECT_NAME} PRIVATE /Ob0)
    endif()
elseif (WIN32 AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wa,-mbig-obj)
endif()


# install the library and generate export set (note that no
# targets file is generated or installed in this step)
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries( ${PROJECT_NAME} INTERFACE
    spdlog::spdlog
)

# install the public headers
install(DIRECTORY ../include/streaming_protocol DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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

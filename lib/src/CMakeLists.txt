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
    BaseSynchronousSignal.hpp
    ProducerSession.hpp
    Server.hpp
    StreamWriter.h
    SynchronousSignal.hpp
)


prepend_include(${PROJECT_NAME} STREAMING_PROTOCOL_STREAMING_INTERFACE_HEADERS)

set(STREAMING_PROTOCOL_STREAMING_LIB_SOURCES
    ${STREAMING_PROTOCOL_STREAMING_INTERFACE_HEADERS}
    # common
    Logging.cpp
    TimeResolution.cpp
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
    BaseSynchronousSignal.cpp
    ControlServer.cpp
    ProducerSession.cpp
    Server.cpp
    StreamWriter.cpp
    SynchronousSignal.cpp
)

add_library(${PROJECT_NAME} STATIC ${STREAMING_PROTOCOL_STREAMING_LIB_SOURCES})
add_library(daq::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# required if this library will ever be linked into a shared object
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json
                                             daq::stream
                                             spdlog
                                             fmt
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

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

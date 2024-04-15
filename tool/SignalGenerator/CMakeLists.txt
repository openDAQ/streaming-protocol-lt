cmake_minimum_required(VERSION 3.11)

project("Signal Generator" VERSION 1.2.0)

# Adding nanoseconds to a point in time does work on Linux only
# Other platforms convert nanoseconds to microseconds and loose some accuracy
# Has an effect for high output rate or when choosing a short processing period (high interrupt rate).
option(FEATURE_SIGNAL_GENERATOR_NANOSECOND_RESOLUTION "Time resolution in nanoseconds" OFF)


# common sources used by all flavors of the signal generator
set(SIGGEN_COMMON_SOURCES
  defines.h
  function.h
  generatedasynchronoussignal.h
  generatedsynchronoussignal.h
  generatedtimesignal.h
  signaldefinition.cpp
  signaldefinition.h
  signalgenerator.cpp
  signalgenerator.h
)

# common source objects with the actual signal generator logic
add_library(siggenCommon OBJECT ${SIGGEN_COMMON_SOURCES})

if(FEATURE_SIGNAL_GENERATOR_NANOSECOND_RESOLUTION)
    target_compile_definitions(siggenCommon PUBLIC _SIGNAL_GENERATOR_NANOSECOND_RESOLUTION)
else()
    target_compile_definitions(siggenCommon PUBLIC _SIGNAL_GENERATOR_MICROSECOND_RESOLUTION)
endif()

target_link_libraries(siggenCommon PUBLIC daq::streaming_protocol daq::stream nlohmann_json::nlohmann_json spdlog::spdlog fmt::fmt)

if (NOT WIN32)
    # FileStream is not supported on win32
    set(SIGGEN2FILE_SOURCES
      siggen2file.cpp
    )

    add_executable(siggen2file
      ${SIGGEN2FILE_SOURCES}
    )
endif()

set(SIGGEN2SOCKET_SOURCES
  siggen2socket.cpp
  worker.cpp
  worker.h
)

add_executable(siggen2socket
  ${SIGGEN2SOCKET_SOURCES}
)

set(SIGGEN2WEBSOCKET_SOURCES
  siggen2websocket.cpp
  worker.cpp
  worker.h
)

add_executable(siggen2websocket
  ${SIGGEN2WEBSOCKET_SOURCES}
)

if (NOT WIN32)
    set(SIGGEN_TARGETS siggen2file
                       siggen2socket
                       siggen2websocket
    )
else()
    set(SIGGEN_TARGETS siggen2socket
                       siggen2websocket
    )
endif()

foreach(tgt ${SIGGEN_TARGETS})
#    target_include_directories(${tgt} PRIVATE "../../include")
    target_compile_features(${tgt} PRIVATE cxx_std_17)
    target_link_libraries(${tgt} PRIVATE
      ${CMAKE_THREAD_LIBS_INIT}
      siggenCommon
      daq::streaming_protocol
      daq::stream
      nlohmann_json::nlohmann_json
    )


    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
      target_link_libraries( ${tgt} PRIVATE
        ws2_32
        wsock32
      )
    endif()

    install(TARGETS ${tgt} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()



if(FEATURE_POST_BUILD_UNITTEST)
  enable_testing()
  add_subdirectory("test")
endif()








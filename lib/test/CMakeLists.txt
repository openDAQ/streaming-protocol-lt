# target_link_options requires cmake >= 3.13
cmake_minimum_required(VERSION 3.13)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project("streaming_protocol_test")

enable_testing()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(GCOV_COMPILE_FLAGS -fno-omit-frame-pointer -fno-optimize-sibling-calls -ftest-coverage -fprofile-arcs)
  set(GCOV_LINK_FLAGS -fprofile-arcs -ftest-coverage)
  set(GCOV_LIB gcov)

  if (NOT MINGW)
      set(ASAN_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
      set(ASAN_LIB asan)
  endif()
endif()

add_custom_target(coverage
  COMMAND echo "=================== Run ===================="
  COMMAND ${CMAKE_MAKE_PROGRAM} test ARGS="--output-on-failure"
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_target_properties(coverage
  PROPERTIES
  ADDITIONAL_CLEAN_FILES "${CMAKE_BINARY_DIR}/*.html"
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES coverage)

enable_testing()

# The "device under test"
set(TEST_LIB_SOURCES
    # common
    ../src/Unit.cpp

    # consumer
    ../src/Controller.cpp
    ../src/HttpPost.cpp
    ../src/MetaInformation.cpp
    ../src/ProtocolHandler.cpp
    ../src/SignalContainer.cpp
    ../src/StreamMeta.cpp
    ../src/SubscribedSignal.cpp

    # producer
    ../src/AsynchronousSignal.cpp
    ../src/BaseSignal.cpp
    ../src/BaseSynchronousSignal.cpp
    ../src/ControlServer.cpp
    ../src/ProducerSession.cpp
    ../src/Server.cpp
    ../src/StreamWriter.cpp
    ../src/SynchronousSignal.cpp
)

set(STREAMING_CLIENT_TEST_LIB streaming_client_test_lib)
add_library(${STREAMING_CLIENT_TEST_LIB} OBJECT ${TEST_LIB_SOURCES})

target_compile_options(${STREAMING_CLIENT_TEST_LIB} PRIVATE
 $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
  -Wall -Werror ${GCOV_COMPILE_FLAGS} ${ASAN_COMPILE_FLAGS}>
)
set_target_properties(${STREAMING_CLIENT_TEST_LIB} PROPERTIES
  CXX_STANDARD_REQUIRED ON
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
)

target_link_options(${STREAMING_CLIENT_TEST_LIB} PRIVATE ${GCOV_LINK_FLAGS} )
target_link_libraries(
  ${STREAMING_CLIENT_TEST_LIB}
  PUBLIC ${ASAN_LIB}
         ${GCOV_LIB}
         nlohmann_json::nlohmann_json
         daq::stream
         spdlog
         fmt
         daq::streaming_protocol
)

target_include_directories(${STREAMING_CLIENT_TEST_LIB} PRIVATE ../include)

target_compile_definitions(${STREAMING_CLIENT_TEST_LIB} PUBLIC
    _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    BOOST_ALL_NO_LIB
)

if (WIN32 AND MSVC)
    target_compile_options(${STREAMING_CLIENT_TEST_LIB} PRIVATE /bigobj)
    if(${MSVC_VERSION} LESS 1920)
        target_compile_options(${STREAMING_CLIENT_TEST_LIB} PRIVATE /Ob0)
    endif()
elseif(WIN32 AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${STREAMING_CLIENT_TEST_LIB} PRIVATE -Wa,-mbig-obj)
endif()

add_executable( StreamMeta.test
    StreamMetaTest.cpp
)

add_executable( SignalContainer.test
    SignalContainerTest.cpp
)

add_executable( SubscribedSignal.test
    SubscribedSignalTest.cpp
)

add_executable( Signal.test
    SignalTest.cpp
)

if (NOT WIN32)
    # FileStream is used here which is not supported under windows
    add_executable( ProducerSession.test
        ProducerSessionTest.cpp
    )
endif()

add_executable( ProtocolHandlerTest.test
    ProtocolHandlerTest.cpp
)

add_executable( StreamWriter.test
    StreamWriterTest.cpp
)

add_executable( Control.Test
    ControlTest.cpp
)

add_executable( Server.test
    ServerTest.cpp
)
add_executable( CompleteWebsocketSession.test
	CompleteWebsocketSessionTest.cpp
)

get_property(targets DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
foreach(tgt ${targets})
  get_target_property(target_type ${tgt} TYPE)
  if (target_type STREQUAL "EXECUTABLE")
    target_compile_definitions(${tgt} PRIVATE _FB_SIMULATION)
    target_compile_options(${tgt} PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall -Werror ${GCOV_COMPILE_FLAGS} ${ASAN_COMPILE_FLAGS}>
    )
    # requires cmake 3.13
    target_link_libraries(
      ${tgt}
      ${ASAN_LIB}
      ${GCOV_LIB}
      GTest::GTest GTest::Main
      ${STREAMING_CLIENT_TEST_LIB}
    )

    set_target_properties(${tgt} PROPERTIES
      CXX_STANDARD_REQUIRED ON
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF
    )
    add_test(NAME daq.streaming-protocol.${tgt} COMMAND ${tgt})
    add_dependencies(coverage ${tgt})

  endif()
endforeach()

set(COMMON_BRANCH_OPTIONS "--exclude-unreachable-branches" "--exclude-throw-branches")
# exclude tests and external library code form coverage
# note: cmake replaces ' ' in string with '\ ' creating a list solves this problem; add --branches to use branch coverage again
set(EXCLUDED_DIRECTORIES "-e${CMAKE_SOURCE_DIR}/build" "-e${CMAKE_SOURCE_DIR}/external" "-e${CMAKE_SOURCE_DIR}/test" "-e${CMAKE_BINARY_DIR}/_deps")

add_custom_command(TARGET coverage
  COMMAND gcovr -r ${CMAKE_SOURCE_DIR} ${EXCLUDED_DIRECTORIES} --object-dir=${CMAKE_BINARY_DIR} ${COMMON_BRANCH_OPTIONS} --html-details -o ${CMAKE_BINARY_DIR}/coverage.html
  COMMAND gcovr -r ${CMAKE_SOURCE_DIR} ${EXCLUDED_DIRECTORIES} --object-dir=${CMAKE_BINARY_DIR} ${COMMON_BRANCH_OPTIONS} --json -o ${CMAKE_BINARY_DIR}/coverage.json
  COMMAND gcovr -r ${CMAKE_SOURCE_DIR} ${EXCLUDED_DIRECTORIES} --object-dir=${CMAKE_BINARY_DIR} ${COMMON_BRANCH_OPTIONS}
  )

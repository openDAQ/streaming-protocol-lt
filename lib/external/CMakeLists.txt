opendaq_get_cmake_mode(_STREAMING_PROTOCOL_CMAKE_MODERN_MODE_SAVED)
opendaq_set_cmake_mode(ANCIENT)
set(CMAKE_FOLDER "external")

option(STREAMING_PROTOCOL_ALWAYS_FETCH_DEPS Off)

list(APPEND CMAKE_MESSAGE_CONTEXT external)

add_subdirectory(nlohman_json)
add_subdirectory(stream)
add_subdirectory(fmt)
add_subdirectory(spdlog)

if (STREAMING_PROTOCOL_POST_BUILD_UNITTEST)
    add_subdirectory(gtest)
endif()

opendaq_set_cmake_mode(${_STREAMING_PROTOCOL_CMAKE_MODERN_MODE_SAVED})

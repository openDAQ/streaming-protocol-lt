# option global for find_package was introduced with 3.24
cmake_minimum_required(VERSION 3.24)

set_cmake_folder_context(TARGET_FOLDER_NAME)

set(stream_REQUIREDVERSION "1.0.1")
if (NOT STREAMING_PROTOCOL_ALWAYS_FETCH_DEPS)
    message(STATUS "Looking for preinstalled stream")
    find_package(stream ${stream_REQUIREDVERSION} GLOBAL QUIET)
endif()

if (stream_FOUND)
    message(STATUS "Found stream: ${stream_VERSION} ${stream_CONFIG}")

    set_target_properties(daq::stream PROPERTIES IMPORTED_GLOBAL TRUE)
else()
    include(FetchContent)
    get_custom_fetch_content_params(stream FC_PARAMS)

    message(STATUS "Fetching stream ${stream_REQUIREDVERSION}...")
    FetchContent_Declare(
            stream
            GIT_REPOSITORY ${OPENDAQ_REPO_PREFIX}/libstream.git
            GIT_TAG v${stream_REQUIREDVERSION}
            GIT_SHALLOW TRUE
            GIT_PROGRESS ON
            GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
            ${FC_PARAMS}
    )
    FetchContent_MakeAvailable(stream)
endif(stream_FOUND)

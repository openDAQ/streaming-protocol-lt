set_cmake_folder_context(TARGET_FOLDER_NAME)

set(fmt_REQUIREDVERSION "7.0.3")
if (NOT STREAMING_PROTOCOL_ALWAYS_FETCH_DEPS)
    message(STATUS "Looking for preinstalled fmt")
    find_package(fmt GLOBAL QUIET ${fmt_REQUIREDVERSION} EXACT)
endif()
if(fmt_FOUND)
    message(STATUS "Found fmt: ${fmt_VERSION} ${fmt_CONFIG}")
else()
    message(STATUS "Fetching fmt version ${fmt_REQUIREDVERSION}")
    include(FetchContent)
    get_custom_fetch_content_params(fmt FC_PARAMS)
    FetchContent_Declare(fmt
                     GIT_REPOSITORY https://github.com/fmtlib/fmt.git
                     GIT_TAG ${fmt_REQUIREDVERSION}
                     GIT_PROGRESS ON
                     GIT_SHALLOW ON
                     GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
                     ${FC_PARAMS}
    )
    set(FMT_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(fmt)
endif()

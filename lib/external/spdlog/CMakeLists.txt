set_cmake_folder_context(TARGET_FOLDER_NAME)


set(spdlog_REQUIREDVERSION "1.8.0")

if (NOT STREAMING_PROTOCOL_ALWAYS_FETCH_DEPS)
    message(STATUS "Looking for preinstalled spdlog")
    find_package(spdlog GLOBAL QUIET ${spdlog_REQUIREDVERSION})
endif()
if(spdlog_FOUND)
    message(STATUS "Found spdlog: ${spdlog_VERSION} ${spdlog_CONFIG}")
else()
    message(STATUS "Fetching spdlog version ${spdlog_REQUIREDVERSION}")

    include(FetchContent)
    get_custom_fetch_content_params(spdlog FC_PARAMS)

    set(SPDLOG_INSTALL ON CACHE BOOL "")

    # spdlog is naughty and has an explicit option for building static or
    # shared, rather than relying on BUILD_SHARED_LIBS; this can create
    # mismatched shared-and-static combinations that lead to relocatable
    # code errors in the build.
    if(BUILD_SHARED_LIBS)
        set(SPDLOG_BUILD_SHARED ON CACHE BOOL "")
    endif()

    FetchContent_Declare(spdlog
                         GIT_REPOSITORY https://github.com/gabime/spdlog.git
                         GIT_TAG v${spdlog_REQUIREDVERSION}
                         GIT_PROGRESS ON
                         GIT_SHALLOW ON
                         GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
                         ${FC_PARAMS}
    )

    FetchContent_MakeAvailable(spdlog)
endif()

# option global for find_package was introduced with 3.24
cmake_minimum_required(VERSION 3.24)

set_cmake_folder_context(TARGET_FOLDER_NAME)

set(nlohmann_json_REQUIREDVERSION "3.10.5")
if (NOT STREAMING_PROTOCOL_ALWAYS_FETCH_DEPS)
    message(STATUS "Looking for preinstalled nlohmann_json")
    find_package(nlohmann_json GLOBAL QUIET ${nlohmann_json_REQUIREDVERSION})
endif()

if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json: ${nlohmann_json_REQUIREDVERSION} ${nlohmann_json_CONFIG}")
    include_directories(PRIVATE ${nlohmann_json})

    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES IMPORTED_GLOBAL TRUE)
else()
    include(FetchContent)
    get_custom_fetch_content_params(nlohmann_json FC_PARAMS)

    message(STATUS "Fetching nlohmann_json ${nlohmann_json_REQUIREDVERSION}...")
    set(JSON_Install ON)
    FetchContent_Declare(nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG v${nlohmann_json_REQUIREDVERSION}
        ${FC_PARAMS}
    )

    FetchContent_MakeAvailable(nlohmann_json)
endif()

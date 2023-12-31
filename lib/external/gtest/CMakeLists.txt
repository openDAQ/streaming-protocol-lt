# option global for find_package was introduced with 3.24
cmake_minimum_required(VERSION 3.24)

set_cmake_folder_context(TARGET_FOLDER_NAME)

set(GTest_REQUIREDVERSION "1.12.1")

if (NOT STREAMING_PROTOCOL_ALWAYS_FETCH_DEPS)
    message(STATUS "Looking for preinstalled GTest")
    find_package(GTest GLOBAL ${GTest_REQUIREDVERSION})
endif()
if(GTest_FOUND)
    message(STATUS "Found GTest: ${GTest_VERSION} ${GTest_CONFIG}")
else()
    message(STATUS "Fetching GTest version ${GTest_REQUIREDVERSION}")

    include(FetchContent)
    get_custom_fetch_content_params(GTest FC_PARAMS)

    set(GTest_WITH_POST_BUILD_UNITTEST OFF)
    set(GTest_WITH_TESTS OFF)

    set(BUILD_GMOCK OFF)
    set(INSTALL_GTEST OFF)
    set(gtest_force_shared_crt ON)
    FetchContent_Declare(
            GTest
            URL https://github.com/google/googletest/archive/release-${GTest_REQUIREDVERSION}.zip
            URL_HASH SHA256=24564e3b712d3eb30ac9a85d92f7d720f60cc0173730ac166f27dda7fed76cb2
            ${FC_PARAMS}
    )

    FetchContent_MakeAvailable(GTest)
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::Main ALIAS gtest_main)
endif()

# option global for find_package was introduced with 3.24
cmake_minimum_required(VERSION 3.24)
set_cmake_folder_context(TARGET_FOLDER_NAME)
project(streaming_client VERSION 0.0.1 LANGUAGES CXX)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

find_package( Threads )

set(CLIENT_SOURCES StreamingClient.cpp)

add_executable(streaming_client ${CLIENT_SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

if (NOT TARGET Boost::program_options)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_LIBS ON)
    find_package(Boost 1.71 REQUIRED COMPONENTS program_options)
    message(STATUS "Boost ${Boost_VERSION_STRING} at: ${Boost_INCLUDE_DIRS} | ${Boost_LIBRARY_DIRS}")

    # Shim a 'Boost::program_options' target
    add_library(Boost_program_options INTERFACE IMPORTED GLOBAL)
    if(NOT TARGET Boost_program_options)
        add_library(Boost::program_options ALIAS Boost_program_options)
    endif()
    
    target_compile_definitions(Boost_program_options INTERFACE BOOST_ALL_NO_LIB)
    target_link_libraries(Boost_program_options INTERFACE ${Boost_LIBRARIES})
    target_include_directories(Boost_program_options INTERFACE ${Boost_INCLUDE_DIRS})
endif()

target_link_libraries(streaming_client PRIVATE Threads::Threads
                                               daq::streaming_protocol
                                               nlohmann_json::nlohmann_json
                                               Boost::program_options
                                               spdlog::spdlog
                                               fmt::fmt)


if (MSVC)
    target_compile_definitions(streaming_client PRIVATE
        _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
        BOOST_ALL_NO_LIB
    )
endif()

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

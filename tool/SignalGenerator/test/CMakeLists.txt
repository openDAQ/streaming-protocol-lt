cmake_minimum_required(VERSION 3.8)
project(time_test LANGUAGES CXX)

enable_testing()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(FEATURE_GCOV)
	SET(CMAKE_CXX_FLAGS "-ftest-coverage -fprofile-arcs ${CMAKE_CXX_FLAGS}")
	SET(EXTRA_LINK_FLAGS "-fprofile-arcs")
    endif()
endif()


find_package(GTest REQUIRED)


set(COMMON
    ../asynchronoussignal.cpp
    ../basesignal.cpp
    ../factorization.cpp
    ../function.cpp
    ../signaldefinition.cpp
    ../signalgenerator.cpp
    ../stdoutwriter.cpp
    ../synchronoussignal.cpp
    ../timefamily.cpp
)


add_library(common OBJECT ${COMMON})


add_executable(addsignals.test addsignals_test.cpp)
add_executable(factorization.test factorization_test.cpp)
add_executable(function.test function_test.cpp)
add_executable(signalgenerator.test signalgenerator_test.cpp)
add_executable(timefamily.test timefamily_test.cpp)



get_property(targets DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
foreach(tgt ${targets})
  get_target_property(target_type ${tgt} TYPE)
  if (target_type STREQUAL "EXECUTABLE")
    target_link_libraries(${tgt} common GTest::GTest GTest::Main gcov)
    set_target_properties(${tgt} PROPERTIES
      LINK_FLAGS "${EXTRA_LINK_FLAGS}"
      CXX_STANDARD_REQUIRED ON
      CXX_STANDARD 17
      CXX_EXTENSIONS OFF
    )
    add_test(NAME daq.streaming_protocol.${tgt} COMMAND ${tgt}
      --report_level=no
      --log_level=all
    )
  endif()
endforeach()




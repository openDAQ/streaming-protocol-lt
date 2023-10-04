cmake_minimum_required(VERSION 3.11)

project("Streaming Protocol Tools" VERSION 0.0.0)


add_executable(calctimefamily calctimefamily.cpp)
add_executable(calcfrequency calcfrequency.cpp)



foreach(tgt calctimefamily calcfrequency)
  target_link_libraries(${tgt} streaming_protocol)
  target_include_directories(${tgt} PRIVATE "../include")

  set_target_properties(${tgt} PROPERTIES
    LINK_FLAGS "${EXTRA_LINK_FLAGS}"
    CXX_STANDARD_REQUIRED ON
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
  )
  install(TARGETS ${tgt} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()


add_subdirectory(StreamingClient)
add_subdirectory(SignalGenerator)

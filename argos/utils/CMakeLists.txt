include_directories(${CMAKE_SOURCE_DIR})

add_library(utils SHARED
  log_name_event.h
  log_name_event.cpp)

target_compile_features(utils PRIVATE cxx_std_17)

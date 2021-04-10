add_library(utils SHARED
  log_name.cpp
  param_name.cpp)

target_compile_features(utils PRIVATE cxx_std_17)

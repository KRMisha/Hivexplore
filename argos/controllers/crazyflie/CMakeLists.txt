add_library(crazyflie SHARED
  crazyflie.cpp)

target_compile_features(crazyflie PRIVATE cxx_std_17)

target_link_libraries(crazyflie
  utils
  argos3core_simulator
  argos3plugin_simulator_crazyflie
  argos3plugin_simulator_genericrobot)

add_library(crazyflie SHARED crazyflie.h crazyflie.cpp)
target_link_libraries(crazyflie
  argos3core_simulator
  argos3plugin_simulator_crazyflie
  argos3plugin_simulator_genericrobot)

add_library(hivexplore_loop_functions MODULE
  hivexplore_loop_functions.h
  hivexplore_loop_functions.cpp)

target_compile_features(hivexplore_loop_functions PRIVATE cxx_std_17)

target_link_libraries(hivexplore_loop_functions
  utils
  argos3core_simulator
  argos3plugin_simulator_entities
  argos3plugin_simulator_footbot
  argos3plugin_simulator_qtopengl
  ${ARGOS_QTOPENGL_LIBRARIES})

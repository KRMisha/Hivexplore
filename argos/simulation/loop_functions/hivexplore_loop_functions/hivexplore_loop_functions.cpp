#include "hivexplore_loop_functions.h"

void CHivexploreLoopFunctions::Init(TConfigurationNode& t_tree) {
    std::cout << "Initialized!\n";
}

void CHivexploreLoopFunctions::Reset() {

}

void CHivexploreLoopFunctions::PreStep() {

}

REGISTER_LOOP_FUNCTIONS(CHivexploreLoopFunctions, "hivexplore_loop_functions")

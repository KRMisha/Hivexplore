#ifndef HIVEXPLORE_LOOP_FUNCTIONS_H
#define HIVEXPLORE_LOOP_FUNCTIONS_H

#include <argos3/core/simulator/loop_functions.h>

using namespace argos;

class CHivexploreLoopFunctions : public CLoopFunctions {
public:
    virtual void Init(TConfigurationNode& t_tree) override;
    virtual void Reset() override;
    virtual void PreStep() override;
};

#endif

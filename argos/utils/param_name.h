#ifndef PARAM_NAME_H
#define PARAM_NAME_H

#include <string>

enum class ParamName {
    MissionState,
    IsM1LedOn,
};

const std::string& paramEventToString(const ParamName& param);

#endif

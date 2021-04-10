#ifndef PARAM_NAME_H
#define PARAM_NAME_H

#include <string>

enum class ParamName {
    MissionState,
    IsLedEnabled,
};

const std::string& paramNameToString(const ParamName& param);

#endif

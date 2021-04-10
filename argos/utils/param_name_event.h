#ifndef PARAM_NAME_EVENT_H
#define PARAM_NAME_EVENT_H

#include <string>

enum class ParamNameEvent {
    MissionState,
    IsM1LedOn,
};

const std::string& paramEventToString(const ParamNameEvent& event);

#endif

#include "param_name.h"
#include <unordered_map>

namespace {
    const std::unordered_map<ParamName, std::string> paramNameStrings = {
        {ParamName::MissionState, "missionState"},
        {ParamName::IsLedEnabled, "isLedEnabled"},
    };

    const std::string unknownParamName = "unknown";
} // namespace

const std::string& paramNameToString(ParamName paramName) {
    auto it = paramNameStrings.find(paramName);
    if (it != paramNameStrings.end()) {
        return it->second;
    }
    return unknownParamName;
}

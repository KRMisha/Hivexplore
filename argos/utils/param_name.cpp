#include <unordered_map>
#include "param_name.h"

namespace {
    const std::unordered_map<ParamName, std::string> paramNameStrings = {
        {ParamName::MissionState, "missionState"},
        {ParamName::IsLedEnabled, "isLedEnabled"},
    };
} // namespace

const std::string& paramNameToString(ParamName paramName) {
    auto it = paramNameStrings.find(paramName);
    if (it != paramNameStrings.end()) {
        return it->second;
    }
    return "unknown";
}

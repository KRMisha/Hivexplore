#include <unordered_map>
#include "param_name.h"

namespace {
    const std::unordered_map<ParamName, std::string> paramNameStrings = {
        {ParamName::MissionState, "missionState"},
        {ParamName::IsM1LedOn, "isM1LedOn"},
    };
} // namespace

const std::string& paramNameToString(const ParamName& param) {
    auto it = paramNameStrings.find(param);
    if (it != paramNameStrings.end()) {
        return it->second;
    }
    return "Unknown";
}

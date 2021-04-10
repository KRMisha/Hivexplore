#include <unordered_map>
#include "param_name_event.h"

namespace {
    std::unordered_map<ParamNameEvent, std::string> socketEventStrings = {
        {ParamNameEvent::MissionState, "missionState"},
        {ParamNameEvent::IsM1LedOn, "isM1LedOn"},
    };
} // namespace

const std::string& paramEventToString(const ParamNameEvent& event) {
    auto it = socketEventStrings.find(event);
    if (it != socketEventStrings.end()) {
        return it->second;
    }
    return "Unknown";
}

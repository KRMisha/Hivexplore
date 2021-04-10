#include <unordered_map>
#include "log_name.h"

namespace {
    const std::unordered_map<LogName, std::string> logNameStrings = {
        {LogName::DroneIds, "drone-ids"},
        {LogName::BatteryLevel, "battery-level"},
        {LogName::Orientation, "orientation"},
        {LogName::Position, "position"},
        {LogName::Velocity, "velocity"},
        {LogName::Range, "range"},
        {LogName::Rssi, "rssi"},
        {LogName::DroneStatus, "drone-status"},
        {LogName::Console, "console"},
    };
} // namespace

const std::string& logNameToString(const LogName& log) {
    auto it = logNameStrings.find(log);
    if (it != logNameStrings.end()) {
        return it->second;
    }
    return "unknown";
}

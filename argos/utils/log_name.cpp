#include "log_name.h"
#include <unordered_map>

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

    const std::string unknownLogNameString = "unknown";
} // namespace

const std::string& logNameToString(LogName logName) {
    auto it = logNameStrings.find(logName);
    if (it != logNameStrings.end()) {
        return it->second;
    }
    return unknownLogNameString;
}

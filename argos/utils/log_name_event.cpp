#include <unordered_map>
#include "log_name_event.h"

namespace {
    std::unordered_map<LogNameEvent, std::string> socketEventStrings = {
        {LogNameEvent::DroneIds, "drone-ids"},
        {LogNameEvent::BatteryLevel, "battery-level"},
        {LogNameEvent::Orientation, "orientation"},
        {LogNameEvent::Position, "position"},
        {LogNameEvent::Velocity, "velocity"},
        {LogNameEvent::Range, "range"},
        {LogNameEvent::Rssi, "rssi"},
        {LogNameEvent::DroneStatus, "drone-status"},
        {LogNameEvent::Console, "console"},
    };
} // namespace

const std::string& logEventToString(const LogNameEvent& event) {
    auto it = socketEventStrings.find(event);
    if (it != socketEventStrings.end()) {
        return it->second;
    }
    return "Unknown";
}

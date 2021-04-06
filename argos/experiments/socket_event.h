#ifndef SOCKET_EVENT_H
#define SOCKET_EVENT_H

#include <string>
#include <unordered_map>

enum class SocketEvent {
    DroneIds,
    BatteryLevel,
    Orientation,
    Position,
    Velocity,
    Range,
    Rssi,
    DroneStatus,
    Console,
};

std::unordered_map<SocketEvent, std::string> socketEventStrings = {
    {SocketEvent::DroneIds, "drone-ids"},
    {SocketEvent::BatteryLevel, "battery-level"},
    {SocketEvent::Orientation, "orientation"},
    {SocketEvent::Position, "position"},
    {SocketEvent::Velocity, "velocity"},
    {SocketEvent::Range, "range"},
    {SocketEvent::Rssi, "rssi"},
    {SocketEvent::DroneStatus, "drone-status"},
    {SocketEvent::Console, "console"},
};

// TODO: Better way to do this?
const std::string& eventToString(const SocketEvent& event) {
    auto it = socketEventStrings.find(event);
    if (it !== socketEventStrings.end()) {
        return it->second;
    }
    return "Unknown";

    // switch (event) {
    // case SocketEvent::DroneIds:
    //     return "drone-ids";
    //     break;
    // case SocketEvent::BatteryLevel:
    //     return "battery-level";
    //     break;
    // case SocketEvent::Orientation:
    //     return "orientation";
    //     break;
    // case SocketEvent::Position:
    //     return "position";
    //     break;
    // case SocketEvent::Velocity:
    //     return "velocity";
    //     break;
    // case SocketEvent::Range:
    //     return "range";
    //     break;
    // case SocketEvent::Rssi:
    //     return "rssi";
    //     break;
    // case SocketEvent::DroneStatus:
    //     return "drone-status";
    //     break;
    // case SocketEvent::Console:
    //     return "console";
    //     break;
    // default:
    //     return "";
    // }
}

// static const char* eventToString[] = {
//     "drone-ids",
//     "battery-level",
//     "orientation",
//     "position",
//     "velocity",
//     "range",
//     "rssi",
//     "drone-status",
//     "console",
// };

// std::map<SocketEvent, const char*> EventToString;
// map_init(EventToString)
//     (SocketEvent::DroneIds, "drone-ids")
//     (SocketEvent::BatteryLevel, "battery-level")
//     (SocketEvent::Orientation, "orientation")
//     (SocketEvent::Position, "position")
//     (SocketEvent::Velocity, "velocity")
//     (SocketEvent::Range, "range")
//     (SocketEvent::Rssi, "rssi")
//     (SocketEvent::DroneStatus, "drone-status")
//     (SocketEvent::Console, "console")
//     ;

#endif

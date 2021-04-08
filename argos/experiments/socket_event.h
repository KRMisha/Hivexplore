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

// TODO: place function in cpp file
const std::string eventToString(const SocketEvent& event) {
    auto it = socketEventStrings.find(event);
    if (it != socketEventStrings.end()) {
        return it->second;
    }
    return "Unknown";
}

#endif

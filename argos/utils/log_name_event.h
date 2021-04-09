#ifndef LOG_NAME_EVENT_H
#define LOG_NAME_EVENT_H

#include <string>

enum class LogNameEvent {
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

const std::string logEventToString(const LogNameEvent& event);

#endif

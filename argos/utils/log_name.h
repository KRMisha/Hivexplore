#ifndef LOG_NAME_H
#define LOG_NAME_H

#include <string>

enum class LogName {
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

const std::string& logEventToString(const LogName& log);

#endif

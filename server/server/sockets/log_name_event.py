import enum


# Use non-standard enum names to match argos enum
# pylint: disable=invalid-name
class LogNameEvent(enum.Enum):
    DroneIds = 'drone-ids'
    BatteryLevel = 'battery-level'
    Orientation = 'orientation'
    Position = 'position'
    Velocity = 'velocity'
    Range = 'range'
    Rssi = 'rssi'
    DroneStatus = 'drone-status'
    Console = 'console'
